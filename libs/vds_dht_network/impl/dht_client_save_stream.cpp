/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "dht_client_save_stream.h"
#include "db_model.h"
#include "chunk_dbo.h"
#include "sync_replica_map_dbo.h"
#include "dht_network_client_p.h"

vds::dht::network::client_save_stream::client_save_stream(client_save_stream&& original) noexcept
: sp_(original.sp_) {
  for (int i = 0; i < vds::dht::network::service::GENERATE_HORCRUX; ++i) {
    this->generators_[i] = std::move(original.generators_[i]);
  }
}

vds::expected<vds::dht::network::client_save_stream> vds::dht::network::client_save_stream::create(
  const vds::service_provider* sp,
  std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> & generators) {

  generator_info_t generator[vds::dht::network::service::GENERATE_HORCRUX];

  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    GET_EXPECTED(item, generator_info_t::create(sp, *generators.find(replica)->second.get()));
    generator[replica] = std::move(item);
  }

  return client_save_stream(sp, std::move(generator));
}

vds::async_task<vds::expected<void>> vds::dht::network::client_save_stream::write_async(const uint8_t* data, size_t len) {
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    CHECK_EXPECTED_ASYNC(co_await this->generators_[replica].generator_->write_async(data, len));
  }

  co_return expected<void>();
}

vds::async_task<vds::expected<std::vector<vds::const_data_buffer>>> vds::dht::network::client_save_stream::save() {

  auto result = std::make_shared<std::vector<const_data_buffer>>(service::GENERATE_HORCRUX);

  std::list<std::function<async_task<expected<void>>()>> final_tasks;

  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction([pthis_ = this->shared_from_this(), result, &final_tasks](class database_transaction & t) -> expected<void> {
    auto pthis = static_cast<client_save_stream *>(pthis_.get());
    for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
      auto & f = static_cast<file_stream_output_async *>(pthis->generators_[replica].hash_stream_->target().get())->target();
      f.close();

      const auto replica_hash = pthis->generators_[replica].hash_stream_->signature();

      orm::chunk_dbo t1;
      orm::sync_replica_map_dbo t2;
      GET_EXPECTED(st, t.get_reader(t1.select(t1.object_id).where(t1.object_id == replica_hash)));
      GET_EXPECTED(st_execute, st.execute());
      if (!st_execute) {
        auto client = pthis->sp_->get<dht::network::client>();
        GET_EXPECTED(size, file::length(f.name()));
        CHECK_EXPECTED(_client::save_data(pthis->sp_, t, replica_hash, f.name()));

        CHECK_EXPECTED(t.execute(
          t1.insert(
            t1.object_id = replica_hash,
            t1.last_sync = std::chrono::system_clock::now() - std::chrono::hours(24)
          )));
        for (uint16_t distributed_replica = 0; distributed_replica < service::GENERATE_DISTRIBUTED_PIECES; ++distributed_replica) {
          CHECK_EXPECTED(t.execute(
            t2.insert(
              t2.object_id = replica_hash,
              t2.node = client->current_node_id(),
              t2.replica = distributed_replica,
              t2.last_access = std::chrono::system_clock::now()
            )));
        }

        CHECK_EXPECTED((*client)->sync_process_.add_sync_entry(t, final_tasks, replica_hash, size));
      }
      else {
        CHECK_EXPECTED(file::delete_file(f.name()));
      }

      (*result)[replica] = replica_hash;
    }
    return expected<void>();
  }));

  while(!final_tasks.empty()) {
    CHECK_EXPECTED_ASYNC(co_await final_tasks.front()());
    final_tasks.pop_front();
  }

  co_return *result;
}

vds::dht::network::client_save_stream::generator_info_t::generator_info_t(
  std::shared_ptr<hash_stream_output_async> && hash_stream,
  vds::chunk_generator<uint16_t> & generator)
: hash_stream_(std::move(hash_stream)),
  generator_(
    std::make_shared<vds::chunk_output_async<uint16_t>>(
      generator,
      this->hash_stream_)){
}

vds::dht::network::client_save_stream::client_save_stream(const service_provider* sp,
  generator_info_t (&& generators)[vds::dht::network::service::GENERATE_HORCRUX])
: sp_(sp) {
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    this->generators_[replica] = std::move(generators[replica]);
  }
}

vds::dht::network::client_save_stream::generator_info_t::generator_info_t(generator_info_t&& original) noexcept 
: hash_stream_(std::move(original.hash_stream_)), generator_(std::move(original.generator_)) {
}

vds::expected<vds::dht::network::client_save_stream::generator_info_t> vds::dht::network::client_save_stream::generator_info_t::create(
  const vds::service_provider* sp,
  vds::chunk_generator<uint16_t>& generator){

  GET_EXPECTED(f, file_stream_output_async::create_tmp(sp));
  GET_EXPECTED(hash_stream, hash_stream_output_async::create(hash::sha256(), f));

  return generator_info_t(std::move(hash_stream), generator);
}

