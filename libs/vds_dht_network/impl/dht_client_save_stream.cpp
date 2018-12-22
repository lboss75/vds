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

vds::dht::network::client_save_stream::client_save_stream(
  const vds::service_provider* sp,
  std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> & generators)
: sp_(sp) {
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    this->generators_[replica].reset(new generator_info_t(sp, *generators.find(replica)->second.get()));
  }
}

vds::async_task<void> vds::dht::network::client_save_stream::write_async(const uint8_t* data, size_t len) {
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    co_await this->generators_[replica]->generator_->write_async(data, len);
  }
}

vds::async_task<std::vector<vds::const_data_buffer>> vds::dht::network::client_save_stream::save() {

  auto result = std::make_shared<std::vector<const_data_buffer>>(service::GENERATE_HORCRUX);

  co_await this->sp_->get<db_model>()->async_transaction([pthis_ = this->shared_from_this(), result](class database_transaction & t) {
    auto pthis = static_cast<client_save_stream *>(pthis_.get());
    for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
      pthis->generators_[replica]->f_.close();
      const auto replica_hash = pthis->generators_[replica]->hash_stream_->signature();

      orm::chunk_dbo t1;
      orm::sync_replica_map_dbo t2;
      auto st = t.get_reader(t1.select(t1.object_id).where(t1.object_id == replica_hash));
      if (!st.execute()) {
        auto client = pthis->sp_->get<dht::network::client>();
        auto size = file::length(pthis->generators_[replica]->f_.name());
        _client::save_data(pthis->sp_, t, replica_hash, pthis->generators_[replica]->f_.name());

        t.execute(
          t1.insert(
            t1.object_id = replica_hash,
            t1.last_sync = std::chrono::system_clock::now() - std::chrono::hours(24)
          ));
        for (uint16_t distributed_replica = 0; distributed_replica < service::GENERATE_DISTRIBUTED_PIECES; ++distributed_replica) {
          t.execute(
            t2.insert(
              t2.object_id = replica_hash,
              t2.node = client->current_node_id(),
              t2.replica = distributed_replica,
              t2.last_access = std::chrono::system_clock::now()
            ));
        }

        (*client)->sync_process_.add_sync_entry(t, replica_hash, size).get();
      }
      else {
        file::delete_file(pthis->generators_[replica]->f_.name());
      }

      (*result)[replica] = replica_hash;
    }
  });

  co_return *result;
}

vds::dht::network::client_save_stream::generator_info_t::generator_info_t(
  const vds::service_provider* sp,
  vds::chunk_generator<uint16_t>& generator):
  f_(vds::file::create_temp(sp)) {
  this->hash_stream_ = std::make_shared<vds::hash_stream_output_async>(
    hash::sha256(),
    std::make_shared<vds::file_stream_output_async>(&this->f_));

  this->generator_ = std::make_shared<vds::chunk_output_async<uint16_t>>(
    generator,
    this->hash_stream_);
}

