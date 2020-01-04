/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "dht_network_client.h"
#include "chunk_dbo.h"
#include "private/dht_network_client_p.h"
#include "messages/sync_messages.h"
#include "messages/dht_route_messages.h"
#include "deflate.h"
#include "db_model.h"
#include "inflate.h"
#include "vds_exceptions.h"
#include "local_data_dbo.h"
#include "well_known_node_dbo.h"
#include "dht_network.h"
#include "sync_replica_map_dbo.h"
#include "dht_client_save_stream.h"
#include "chunk_replica_data_dbo.h"
#include "transaction_block_builder.h"
#include "node_info_dbo.h"
#include "current_config_dbo.h"
#include "device_config_dbo.h"
#include "device_record_dbo.h"
#include "transaction_log.h"
#include "transaction_block.h"

vds::dht::network::client::client()
: is_new_node_(true), port_(0) {
}

vds::expected<std::shared_ptr<vds::dht::network::_client>> vds::dht::network::_client::create(
  const service_provider * sp,
  const std::shared_ptr<iudp_transport> & udp_transport,
  const std::shared_ptr<asymmetric_public_key> & node_public_key) {

  GET_EXPECTED(this_node_id, node_public_key->fingerprint());

  return std::make_shared<_client>(sp, udp_transport, this_node_id);
}

vds::dht::network::_client::_client(
  const service_provider * sp,
  const std::shared_ptr<iudp_transport> & udp_transport,
  const const_data_buffer & this_node_id)
  : sp_(sp),
  route_(sp, this_node_id),
  update_timer_("DHT Network"),
  update_route_table_counter_(0),
  udp_transport_(udp_transport),
  sync_process_(sp),
  update_wellknown_connection_enabled_(true) {
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    this->generators_[replica].reset(new chunk_generator<uint16_t>(service::MIN_HORCRUX, replica));
  }
}

vds::expected<std::vector<vds::const_data_buffer>> vds::dht::network::_client::save(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const const_data_buffer& value) {

  std::vector<const_data_buffer> result(service::GENERATE_HORCRUX);
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    binary_serializer s;
    CHECK_EXPECTED(this->generators_.find(replica)->second->write(s, value.data(), value.size()));
    const auto replica_data = s.move_data();
    GET_EXPECTED(replica_hash, hash::signature(hash::sha256(), replica_data));

    orm::chunk_dbo t1;
    orm::sync_replica_map_dbo t2;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.object_id).where(t1.object_id == replica_hash)));
    GET_EXPECTED(st_execute, st.execute());
    if (!st_execute) {
      auto client = this->sp_->get<dht::network::client>();
      CHECK_EXPECTED(save_data(this->sp_, t, replica_hash, replica_data));
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

      CHECK_EXPECTED(this->sync_process_.add_sync_entry(t, final_tasks, replica_hash, replica_data.size()));
    }
    result[replica] = replica_hash;
  }

  return result;
}


vds::expected<std::shared_ptr<vds::dht::network::client_save_stream>> vds::dht::network::_client::create_save_stream() {
  GET_EXPECTED(result, client_save_stream::create(this->sp_, this->generators_));
  return std::make_shared<client_save_stream>(std::move(result));
}

vds::async_task<vds::expected<bool>> vds::dht::network::_client::apply_message(
  const messages::dht_find_node& message,
  const imessage_map::message_info_t& message_info) {
  std::map<const_data_buffer /*distance*/,
  std::map<const_data_buffer, std::shared_ptr<dht_route::node>>> result_nodes;

  CHECK_EXPECTED_ASYNC(this->route_.search_nodes(
    message.target_id,
    70,
    [&message_info](const dht_route::node & node) -> expected<bool> {
      return message_info.hops().end() == std::find(message_info.hops().begin(), message_info.hops().end(), node.node_id_);
    },
    result_nodes));

  std::list<messages::dht_find_node_response::target_node> result;
  for (auto& presult : result_nodes) {
    for (auto& node : presult.second) {
      result.emplace_back(
        node.second->node_id_,
        node.second->proxy_session_->address().to_string(),
        node.second->hops_);
    }
  }

  if (!result.empty()) {
    this->sp_->get<logger>()->trace(ThisModule, "Send dht_find_node_response");
    CHECK_EXPECTED_ASYNC(co_await this->send(
      message_info.source_node(),
      message_create<messages::dht_find_node_response>(result)));
    co_return true;
  }

  co_return false;
}

vds::async_task<vds::expected<bool>> vds::dht::network::_client::apply_message(
  const messages::dht_find_node_response& message,
  const imessage_map::message_info_t& message_info) {
  for (auto& p : message.nodes) {
    if (
      p.target_id_ != this->current_node_id()
      && this->route_.add_node(
      p.target_id_,
      message_info.session(),
      p.hops_ + message_info.hops().size() + 1,
      true)) {
      CHECK_EXPECTED_ASYNC(co_await this->udp_transport_->try_handshake(p.address_));
    }
  }

  co_return true;
}

vds::async_task<vds::expected<bool>> vds::dht::network::_client::apply_message(
  const messages::dht_ping& message,
  const imessage_map::message_info_t& message_info) {

  this->sp_->get<logger>()->trace(ThisModule, "Send dht_pong");
  CHECK_EXPECTED_ASYNC(co_await message_info.session()->send_message(
    this->udp_transport_,
    (uint8_t)messages::dht_pong::message_id,
    message_info.source_node(),
    message_serialize(messages::dht_pong())));

  co_return true;
}

vds::async_task<vds::expected<bool>> vds::dht::network::_client::apply_message(
  const messages::dht_pong& message,
  const imessage_map::message_info_t& message_info) {
  this->route_.mark_pinged(
    message_info.source_node(),
    message_info.session()->address());
  co_return true;
}

vds::async_task<vds::expected<void>> vds::dht::network::_client::for_near(
  const const_data_buffer& target_node_id,
  size_t max_count,
  const std::function<expected<bool>(const dht_route::node & node)>& filter,
  lambda_holder_t<vds::async_task<vds::expected<bool>>, const std::shared_ptr<dht_route::node>&> callback)
{
  return this->route_.for_near(target_node_id, max_count, filter, std::move(callback));
}

vds::expected<void> vds::dht::network::_client::for_near_sync(
  const const_data_buffer& target_node_id,
  size_t max_count,
  const std::function<expected<bool>(const dht_route::node & node)>& filter,
  lambda_holder_t<vds::expected<bool>, const std::shared_ptr<dht_route::node>&> callback)
{
  return this->route_.for_near_sync(target_node_id, max_count, filter, std::move(callback));
}

vds::async_task<vds::expected<void>> vds::dht::network::_client::add_session(
  const std::shared_ptr<dht_session>& session,
  uint8_t hops) {
  this->route_.add_node(session->partner_node_id(), session, hops, false);
  return this->sp_->get<db_model>()->async_transaction([address = session->address().to_string(), session](database_transaction& t)->expected<void> {
    orm::well_known_node_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.last_connect).where(t1.address == address)));
    GET_EXPECTED(st_execute, st.execute());
    if(st_execute) {
      CHECK_EXPECTED(t.execute(t1.update(t1.last_connect = std::chrono::system_clock::now()).where(t1.address == address)));
    }
    else {
      CHECK_EXPECTED(t.execute(t1.insert(t1.last_connect = std::chrono::system_clock::now(), t1.address = address)));
    }

    orm::node_info_dbo t2;
    GET_EXPECTED_VALUE(st, t.get_reader(t2.select(t2.last_activity).where(t2.node_id == session->partner_node_id())));
    GET_EXPECTED_VALUE(st_execute, st.execute());
    if (st_execute) {
      if (t2.last_activity.get(st) < std::chrono::system_clock::now()) {
        CHECK_EXPECTED(t.execute(t2.update(t2.last_activity = std::chrono::system_clock::now()).where(t2.node_id == session->partner_node_id())));
      }
    }
    else {
      GET_EXPECTED(public_key, session->partner_node_key().der());
      CHECK_EXPECTED(t.execute(t2.insert(t2.last_activity = std::chrono::system_clock::now(), t2.node_id = session->partner_node_id(), t2.public_key = public_key)));
    }

    return expected<void>();
  });
}

vds::async_task<vds::expected<void>> vds::dht::network::_client::send(
  const const_data_buffer& target_node_id,
  const message_type_t message_id,
  expected<const_data_buffer> && message) {
  CHECK_EXPECTED_ERROR(message);

  return this->route_.for_near(
    target_node_id,
    1,
    [](const dht_route::node& node) -> expected<bool> {
      return true;
    },
    [target_node_id, message_id, msg = std::move(message.value()), pthis = this->shared_from_this()](
    const std::shared_ptr<dht_route::node>& candidate) -> async_task<expected<bool>>{
    CHECK_EXPECTED_ASYNC(co_await candidate->proxy_session_->send_message(
        pthis->udp_transport_,
        (uint8_t)message_id,
        target_node_id,
        msg));
      co_return false;
    });
}

vds::async_task<vds::expected<void>> vds::dht::network::_client::send_near(
  const const_data_buffer& target_node_id,
  size_t max_count,
  const message_type_t message_id,
  expected<const_data_buffer>&& message,
  const std::function<expected<bool>(const dht_route::node & node)>& filter) {
  CHECK_EXPECTED_ERROR(message);

  return this->route_.for_near(
    target_node_id,
    max_count,
    filter,
    [target_node_id, message_id, msg = std::move(message.value()), pthis = this->shared_from_this()](
      const std::shared_ptr<dht_route::node>& candidate)->async_task<expected<bool>>{
      CHECK_EXPECTED_ASYNC(co_await candidate->proxy_session_->send_message(
        pthis->udp_transport_,
        (uint8_t)message_id,
        candidate->node_id_,
        msg));
      co_return false;
    });
}


vds::async_task<vds::expected<void>> vds::dht::network::_client::proxy_message(
    const const_data_buffer &target_node_id,
    message_type_t message_id,
    const const_data_buffer &message,
    const std::vector<const_data_buffer>& hops) {

  if (hops.size() > 10) {
    auto best = this->current_node_id();
    for (const auto& item : hops) {
      if (dht_object_id::distance(target_node_id, item) < dht_object_id::distance(target_node_id, best)) {
        best = item;
      }
    }

    return this->proxy_message(
      hops[hops.size() - 1],
      message_id,
      message,
      std::vector<const_data_buffer>({ best })
    );
  }
  else {
    return this->route_.for_near(
      target_node_id,
      1,
      [&hops](const dht_route::node& node) -> expected<bool> {
        return hops.end() == std::find(hops.begin(), hops.end(), node.proxy_session_->partner_node_id());
      },
      [
        target_node_id,
        message_id,
        message,
        pthis = this->shared_from_this(),
        distance = dht_object_id::distance(this->current_node_id(), target_node_id),
        hops](
          const std::shared_ptr<dht_route::node>& candidate)->vds::async_task<vds::expected<bool>> {
        pthis->sp_->get<logger>()->trace(
          "dht_protocol",
          "%s Message %d from %s to %s redirected to %s over %s",
          base64::from_bytes(pthis->current_node_id()).c_str(),
          message_id,
          base64::from_bytes(hops[0]).c_str(),
          base64::from_bytes(target_node_id).c_str(),
          base64::from_bytes(candidate->node_id_).c_str(),
          candidate->proxy_session_->address().to_string().c_str());

        CHECK_EXPECTED_ASYNC(co_await candidate->proxy_session_->proxy_message(
          pthis->udp_transport_,
          (uint8_t)message_id,
          target_node_id,
          hops,
          message));
        co_return false;
      });
  }
}

vds::async_task<vds::expected<void>> vds::dht::network::_client::send_neighbors(
  const message_type_t message_id,
  expected<const_data_buffer> && message) {
  CHECK_EXPECTED_ERROR(message);
  return this->route_.for_neighbors(
    [message_id, msg = std::move(message.value()), pthis = this->shared_from_this()](
      const std::shared_ptr<dht_route::node>& candidate)->vds::async_task<vds::expected<bool>> {
    CHECK_EXPECTED_ASYNC(co_await candidate->proxy_session_->send_message(
        pthis->udp_transport_,
        (uint8_t)message_id,
        candidate->node_id_,
        msg));
      co_return true;
    });
}

vds::expected<vds::const_data_buffer> vds::dht::network::_client::replica_id(const std::string& key, uint16_t replica) {
  auto id = "{" + std::to_string(replica) + "}" + key;
  return hash::signature(hash::sha256(), id.c_str(), id.length());
}


vds::expected<void> vds::dht::network::_client::start() {
  return this->update_timer_.start(this->sp_, std::chrono::seconds(60), [pthis = this->shared_from_this()]() -> async_task<expected<bool>>{

    CHECK_EXPECTED_ASYNC(co_await pthis->udp_transport_->on_timer());
    CHECK_EXPECTED_ASYNC(co_await pthis->route_.on_timer(pthis->udp_transport_));
    CHECK_EXPECTED_ASYNC(co_await pthis->update_route_table());

    std::list<std::function<async_task<expected<void>>()>> final_tasks;
    CHECK_EXPECTED_ASYNC(co_await pthis->sp_->get<db_model>()->async_transaction([pthis, &final_tasks](database_transaction& t) -> expected<void> {
        return pthis->process_update(t, final_tasks);
      }));

    while(!final_tasks.empty()) {
      CHECK_EXPECTED_ASYNC(co_await final_tasks.front()());
      final_tasks.pop_front();
    }

      co_return !pthis->sp_->get_shutdown_event().is_shuting_down();
  });
}

void vds::dht::network::_client::stop() {
  //this->udp_transport_->stop(sp);
}

void vds::dht::network::_client::get_neighbors(
                                               std::list<std::shared_ptr<dht_route::node>>
                                               & result) {
  this->route_.get_neighbors(result);
}

vds::expected<void> vds::dht::network::_client::on_new_session(
  database_read_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const const_data_buffer& partner_id) {
  return this->sync_process_.on_new_session(
    t,
    final_tasks,
    partner_id);
}

void vds::dht::network::_client::remove_session(
  
  const std::shared_ptr<dht_session>& session) {
  this->route_.remove_session(session);
}

vds::expected<vds::filename> vds::dht::network::_client::save_data(
  const service_provider * sp,
  database_transaction& t,
  const const_data_buffer& data_hash,
  const const_data_buffer& data) {

  auto client = sp->get<network::client>();

  orm::device_record_dbo t4;
  GET_EXPECTED(st, t.get_reader(t4.select(
    t4.storage_path, t4.data_size)
    .where(t4.node_id == client->current_node_id()
      && t4.data_hash == data_hash)));
  GET_EXPECTED(st_execute, st.execute());
  if (st_execute) {
    vds_assert(data.size() == t4.data_size.get(st));
    if (data.size() != t4.data_size.get(st)) {
      return make_unexpected<std::runtime_error>("Data collusion " + base64::from_bytes(data_hash));
    }

    return filename(t4.storage_path.get(st));
  }

  uint64_t allowed_size = 0;
  std::string local_path;

  orm::current_config_dbo t3;
  db_value<int64_t> data_size;
  GET_EXPECTED_VALUE(st, t.get_reader(
    t3.select(t3.local_path, t3.reserved_size, db_sum(t4.data_size).as(data_size))
      .left_join(t4, t4.node_id == t3.node_id && t4.storage_path == t3.local_path)
      .group_by(t3.local_path, t3.reserved_size)));
  WHILE_EXPECTED (st.execute()) {
    const int64_t size = data_size.is_null(st) ? 0 : data_size.get(st);
    if (t3.reserved_size.get(st) > size && allowed_size < (t3.reserved_size.get(st) - size)) {
      allowed_size = (t3.reserved_size.get(st) - size);
      local_path = t3.local_path.get(st);
    }
  }
  WHILE_EXPECTED_END()

  if (local_path.empty() || allowed_size < data.size()) {
    return vds::make_unexpected<std::runtime_error>("No disk space");
  }

  auto append_path = base64::from_bytes(data_hash);
  str_replace(append_path, '+', '#');
  str_replace(append_path, '/', '_');

  foldername fl(local_path);
  CHECK_EXPECTED(fl.create());

  fl = foldername(fl, append_path.substr(0, 10));
  CHECK_EXPECTED(fl.create());

  fl = foldername(fl, append_path.substr(10, 10));
  CHECK_EXPECTED(fl.create());

  filename fn(fl, append_path.substr(20));
  CHECK_EXPECTED(file::write_all(fn, data));

  CHECK_EXPECTED(t.execute(t4.insert(
    t4.node_id = client->current_node_id(),
    t4.storage_path = fn.full_name(),
    t4.data_hash = data_hash,
    t4.data_size = data.size())));

  return fn;
}

vds::expected<vds::filename> vds::dht::network::_client::save_data(const service_provider* sp, database_transaction& t,
  const const_data_buffer& data_hash, const filename& original_file) {
  auto client = sp->get<network::client>();

  uint64_t allowed_size = 0;
  std::string local_path;

  orm::current_config_dbo t3;
  orm::device_record_dbo t4;
  db_value<int64_t> data_size;
  GET_EXPECTED(st, t.get_reader(
    t3.select(t3.local_path, t3.reserved_size, db_sum(t4.data_size).as(data_size))
    .left_join(t4, t4.node_id == t3.node_id && t4.storage_path == t3.local_path)
    .group_by(t3.local_path, t3.reserved_size)));

  WHILE_EXPECTED (st.execute()) {
    const int64_t size = data_size.is_null(st) ? 0 : data_size.get(st);
    if (t3.reserved_size.get(st) > size && allowed_size < (t3.reserved_size.get(st) - size)) {
      allowed_size = (t3.reserved_size.get(st) - size);
      local_path = t3.local_path.get(st);
    }
  }
  WHILE_EXPECTED_END()

  GET_EXPECTED(size, file::length(original_file));
  if (local_path.empty() || allowed_size < size) {
    return vds::make_unexpected<std::runtime_error>("No disk space");
  }

  auto append_path = base64::from_bytes(data_hash);
  str_replace(append_path, '+', '#');
  str_replace(append_path, '/', '_');

  foldername fl(local_path);
  CHECK_EXPECTED(fl.create());

  fl = foldername(fl, append_path.substr(0, 10));
  CHECK_EXPECTED(fl.create());

  fl = foldername(fl, append_path.substr(10, 10));
  CHECK_EXPECTED(fl.create());

  filename fn(fl, append_path.substr(20));
  CHECK_EXPECTED(file::move(original_file, fn));

  CHECK_EXPECTED(t.execute(t4.insert(
    t4.node_id = client->current_node_id(),
    t4.storage_path = fn.full_name(),
    t4.data_hash = data_hash,
    t4.data_size = size)));

  return fn;
}

vds::async_task<vds::expected<void>> vds::dht::network::_client::update_route_table() {
  if (0 == this->update_route_table_counter_) {
    for (size_t i = 0; i < 8 * this->route_.current_node_id().size(); ++i) {
      auto canditate = dht_object_id::generate_random_id(this->route_.current_node_id(), i);
      CHECK_EXPECTED_ASYNC(co_await this->send_neighbors(
        message_create<messages::dht_find_node>(std::move(canditate))));

    }
    this->update_route_table_counter_ = 10;
  }
  else {
    this->update_route_table_counter_--;
  }

  co_return expected<void>();
}

vds::expected<void> vds::dht::network::_client::process_update(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks) {
  CHECK_EXPECTED(this->sync_process_.do_sync(t, final_tasks));
  CHECK_EXPECTED(this->update_wellknown_connection(t, final_tasks));

  return expected<void>();
}

void vds::dht::network::_client::get_route_statistics(route_statistic& result) {
  this->route_.get_statistics(result);
}

void vds::dht::network::_client::get_session_statistics(session_statistic& session_statistic) {
  static_cast<udp_transport *>(this->udp_transport_.get())->get_session_statistics(session_statistic);
}

vds::expected<bool> vds::dht::network::_client::apply_message(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const messages::sync_new_election_request& message,
  const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const messages::sync_new_election_response& message,
  const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_add_message_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_leader_broadcast_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_leader_broadcast_response& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_replica_operations_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_replica_operations_response& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_looking_storage_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_looking_storage_response& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_snapshot_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_snapshot_response& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_offer_send_replica_operation_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_offer_remove_replica_operation_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                                               const messages::sync_replica_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message(
   database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const messages::sync_replica_data& message,
  const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::dht::network::_client::apply_message( database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const messages::sync_replica_query_operations_request& message, const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, final_tasks, message, message_info);
}

vds::async_task<vds::expected<vds::const_data_buffer>> vds::dht::network::_client::restore(
  std::vector<const_data_buffer> object_ids,
  std::chrono::steady_clock::time_point start) {
  auto result = std::make_shared<const_data_buffer>();
    for (;;) {
    uint8_t progress;
    GET_EXPECTED_VALUE_ASYNC(progress, co_await this->restore_async(
      object_ids,
      result));

    if (result->size() > 0) {
      co_return *result;
    }

    if (std::chrono::minutes(30) < (std::chrono::steady_clock::now() - start)) {
      co_return vds::make_unexpected<vds_exceptions::not_found>();
    }

    std::this_thread::sleep_for(std::chrono::seconds(60));
  }
}

vds::expected<vds::dht::network::client::block_info_t> vds::dht::network::_client::prepare_restore(
  database_read_transaction & t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const std::vector<const_data_buffer>& object_ids) {

  auto result = vds::dht::network::client::block_info_t();

    for (const auto& object_id : object_ids) {
      std::list<uint16_t> replicas;
      GET_EXPECTED_VALUE(replicas, this->sync_process_.prepare_restore_replica(t, final_tasks, object_id));
      result.replicas[object_id] = std::move(replicas);
    }

  return result;
}

vds::expected<void> vds::dht::network::_client::restore_async(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const std::vector<const_data_buffer>& object_ids,
  const std::shared_ptr<const_data_buffer>& result,
  const std::shared_ptr<uint8_t> & result_progress) {
  
  std::vector<uint16_t> replicas;
  std::vector<const_data_buffer> datas;
  std::map<uint16_t, const_data_buffer> unknonw_replicas;

  orm::chunk_dbo t1;
  orm::device_record_dbo t4;
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    GET_EXPECTED(st, t.get_reader(
      t1
      .select(t4.storage_path)
      .inner_join(t4, t4.data_hash == t1.object_id)
      .where(t1.object_id == object_ids[replica])));
    GET_EXPECTED(st_execute, st.execute());
    if (st_execute) {
      replicas.push_back(replica);
      GET_EXPECTED(data, file::read_all(filename(t4.storage_path.get(st))));
      datas.push_back(data);

      if (replicas.size() >= service::MIN_HORCRUX) {
        break;
      }
    }
    else {
      unknonw_replicas[replica] = object_ids[replica];
    }
  }

  if (replicas.size() < service::MIN_HORCRUX) {
    for (auto p = unknonw_replicas.begin(); unknonw_replicas.end() != p; ) {
      std::vector<uint16_t> piece_replicas;
      std::vector<const_data_buffer> piece_datas;

      orm::chunk_replica_data_dbo t2;
      GET_EXPECTED(st, t.get_reader(
        t2.select(
          t2.replica, t2.replica_hash, t4.storage_path)
        .inner_join(t4, t4.data_hash == t2.replica_hash)
        .where(t2.object_id == p->second)));
      WHILE_EXPECTED(st.execute()) {
        piece_replicas.push_back(t2.replica.get(st));
        GET_EXPECTED(data,
          _client::read_data(
            t2.replica_hash.get(st),
            filename(t4.storage_path.get(st))));
        piece_datas.push_back(data);

        if (piece_replicas.size() >= service::MIN_DISTRIBUTED_PIECES) {
          break;
        }
      }
      WHILE_EXPECTED_END()

        if (piece_replicas.size() >= service::MIN_DISTRIBUTED_PIECES) {
          chunk_restore<uint16_t> restore(service::MIN_DISTRIBUTED_PIECES, piece_replicas.data());
          GET_EXPECTED(data, restore.restore(piece_datas));
          GET_EXPECTED(sig, hash::signature(hash::sha256(), data));
          if (p->second != sig) {
            return vds::make_unexpected<std::runtime_error>("Invalid error");
          }
          CHECK_EXPECTED(_client::save_data(this->sp_, t, p->second, data));

          this->sp_->get<logger>()->trace(SyncModule, "Restored object %s", base64::from_bytes(p->second).c_str());

          CHECK_EXPECTED(
            t.execute(
              t1.insert(
                t1.object_id = p->second,
                t1.last_sync = std::chrono::system_clock::now())));

          replicas.push_back(p->first);
          datas.push_back(data);

          p = unknonw_replicas.erase(p);
        }
        else {
          ++p;
        }
    }
  }

  if (replicas.size() >= service::MIN_HORCRUX) {
    chunk_restore<uint16_t> restore(service::MIN_HORCRUX, replicas.data());
    GET_EXPECTED(data, restore.restore(datas));
    *result = data;
    *result_progress = 100;
    return expected<void>();
  }

  *result_progress = 99 * replicas.size() / service::MIN_HORCRUX;
  for (const auto replica : unknonw_replicas) {
    CHECK_EXPECTED(this->sync_process_.restore_replica(t, final_tasks, replica.second));
  }

  return expected<void>();
}

vds::async_task<vds::expected<uint8_t>> vds::dht::network::_client::restore_async(
  const std::vector<const_data_buffer>& object_ids,
  const std::shared_ptr<const_data_buffer>& result) {

  auto result_progress = std::make_shared<uint8_t>();
  std::list<std::function<async_task<expected<void>>()>> final_tasks;
  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction(
    [pthis = this->shared_from_this(), object_ids, result, result_progress, &final_tasks](
      database_transaction& t) -> expected<void> {
    return pthis->restore_async(t, final_tasks, object_ids, result, result_progress);
  }));

  while(!final_tasks.empty()) {
    CHECK_EXPECTED_ASYNC(co_await final_tasks.front()());
    final_tasks.pop_front();
  }

  co_return *result_progress;
}

vds::expected<void> vds::dht::network::_client::update_wellknown_connection(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks) {
  if (this->update_wellknown_connection_enabled_) {
    orm::well_known_node_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.address)));
    WHILE_EXPECTED(st.execute()) {
      auto address = t1.address.get(st);
      final_tasks.push_back([this, address]()->async_task<expected<void>> {
        (void)co_await this->udp_transport_->try_handshake(address);
        co_return expected<void>();
      });
    }
    WHILE_EXPECTED_END()
  }
  else {
    final_tasks.push_back([this]()->async_task<expected<void>> {
      (void)co_await this->udp_transport_->try_handshake("udp://localhost:8050");
      co_return expected<void>();
    });
  }

  final_tasks.push_back([this]()->async_task<expected<void>> {
    (void)this->udp_transport_->broadcast_handshake();
    co_return expected<void>();
  });
  return expected<void>();
}

vds::expected<vds::const_data_buffer> vds::dht::network::_client::read_data(
  const const_data_buffer& data_hash,
  const filename& data_path) {
  GET_EXPECTED(data, file::read_all(data_path));
  GET_EXPECTED(data_hash_fact, hash::signature(hash::sha256(), data));
  if(data_hash != data_hash_fact) {
    return make_unexpected<std::runtime_error>("Data is corrupted");
  }

  return data;
}

vds::expected<void> vds::dht::network::_client::delete_data(
  const const_data_buffer& /*replica_hash*/,
  const filename& data_path) {
  return file::delete_file(data_path);
}

void vds::dht::network::_client::add_route(
  const std::vector<const_data_buffer> & nodes,
  const std::shared_ptr<dht_session>& session) {
  for (uint16_t hops = 0; hops < nodes.size(); ++hops) {
    this->add_route(nodes[hops], hops, session);
  }
}

void vds::dht::network::_client::add_route(
  const const_data_buffer& source_node,
  uint16_t hops,
  const std::shared_ptr<dht_session>& session) {
  this->route_.add_node(source_node, session, hops, false);

}

vds::async_task<vds::expected<void>> vds::dht::network::_client::redirect(
  const_data_buffer node_id,
  std::vector<const_data_buffer> hops,
  message_type_t message_id,
  expected<const_data_buffer> message)
{
  CHECK_EXPECTED_ERROR(message);

  return this->route_.for_near(
    node_id,
    1,
    [last_node = hops[0]](const dht_route::node& node)->expected<bool> {
      return last_node != node.proxy_session_->partner_node_id();
    },
    [pthis = this->shared_from_this(), h = std::move(hops), message_id, m = std::move(message)](const std::shared_ptr<dht_route::node> & node) mutable -> vds::async_task<vds::expected<bool>> {
    CHECK_EXPECTED_ASYNC(co_await node->proxy_session_->proxy_message(
      pthis->udp_transport_,
      (uint8_t)message_id,
      node->node_id_,
      std::move(h),
      std::move(m.value())));
    co_return false;
  });
}

vds::async_task<vds::expected<void>> vds::dht::network::_client::find_nodes(
    
    const vds::const_data_buffer &node_id,
    size_t radius) {

  co_return co_await this->send_neighbors(message_create<messages::dht_find_node>(node_id));
}

vds::expected<void> vds::dht::network::client::start(
  const service_provider * sp,
  const std::shared_ptr<iudp_transport> & udp_transport,
  uint16_t port,
  bool dev_network) {

  this->port_ = port;
  
  udp_transport->start(
    sp,
    this->node_public_key_,
    this->node_key_,
    port,
    dev_network).then([](expected<void> result) {
  });
  
  GET_EXPECTED_VALUE(this->impl_, _client::create(sp, udp_transport, this->node_public_key_));
  return this->impl_->start();
}

vds::expected<void> vds::dht::network::client::load_keys(database_transaction & t)
{
  orm::current_config_dbo t1;
  GET_EXPECTED(st, t.get_reader(t1.select(t1.public_key, t1.private_key)));
  GET_EXPECTED(st_execute, st.execute());
  if (st_execute) {
    GET_EXPECTED(node_public_key_data, asymmetric_public_key::parse_der(t1.public_key.get(st)));
    this->node_public_key_ = std::make_shared<asymmetric_public_key>(std::move(node_public_key_data));

    GET_EXPECTED(node_key_data, asymmetric_private_key::parse_der(t1.private_key.get(st), std::string()));
    this->node_key_ = std::make_shared<asymmetric_private_key>(std::move(node_key_data));
  }
  else {
    GET_EXPECTED(node_key_data, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
    this->node_key_ = std::make_shared<asymmetric_private_key>(std::move(node_key_data));

    GET_EXPECTED(public_key, asymmetric_public_key::create(*this->node_key_));
    GET_EXPECTED(key_id, public_key.fingerprint());

    this->node_public_key_ = std::make_shared<asymmetric_public_key>(std::move(public_key));
    GET_EXPECTED(node_public_key_data, this->node_public_key_->der());
    GET_EXPECTED(node_key_der, this->node_key_->der(std::string()));
    CHECK_EXPECTED(t.execute(t1.insert(
      t1.node_id = key_id,
      t1.public_key = node_public_key_data,
      t1.private_key = node_key_der,
      t1.local_path = std::string(),
      t1.owner_id = const_data_buffer(),
      t1.reserved_size = 0)));
  }

  return expected<void>();
}

vds::expected<void> vds::dht::network::client::stop() {
  if (this->impl_) {
    this->impl_->stop();
  }

  return expected<void>();
}

static const uint8_t pack_block_iv[] = {
  // 0     1     2     3     4     5     6     7
  0xa5, 0xbb, 0x9f, 0xce, 0xc2, 0xe4, 0x4b, 0x91,
  0xa8, 0xc9, 0x59, 0x44, 0x62, 0x55, 0x90, 0x24
};

vds::expected<vds::dht::network::client::chunk_info> vds::dht::network::client::save(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const const_data_buffer& data) {

  GET_EXPECTED(key_data, hash::signature(hash::sha256(), data));

  if (key_data.size() != symmetric_crypto::aes_256_cbc().key_size()
    || sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()) {
    return vds::make_unexpected<std::runtime_error>("Design error");
  }

  auto key = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    key_data.data(),
    pack_block_iv);

  GET_EXPECTED(key_data2, hash::signature(
    hash::sha256(),
    symmetric_encrypt::encrypt(key, data)));

  auto key2 = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    key_data2.data(),
    pack_block_iv);

  GET_EXPECTED(zipped, deflate::compress(data));

  GET_EXPECTED(crypted_data, symmetric_encrypt::encrypt(key2, zipped));
  std::vector<vds::const_data_buffer> info;
  GET_EXPECTED_VALUE(info, this->impl_->save(t, final_tasks, crypted_data));
  return chunk_info
  {
    key_data,
    key_data2,
    info
  };
}

vds::expected<vds::const_data_buffer> vds::dht::network::client::save(
  const service_provider * sp,
  transactions::transaction_block_builder & block,
  database_transaction & t)
{
  if (!this->node_public_key_) {
    CHECK_EXPECTED(this->load_keys(t));
  }

  if (this->is_new_node_) {
    GET_EXPECTED(node_id, this->node_public_key_->fingerprint());

    orm::node_info_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.public_key).where(t1.node_id == node_id)));
    GET_EXPECTED(st_result, st.execute());
    if (!st_result) {
      CHECK_EXPECTED(block.add(message_create<transactions::node_add_transaction>(this->node_public_key_)));
    }

    this->is_new_node_ = false;
  }
  //Load state
  GET_EXPECTED(data, transactions::transaction_block::build(t, block.close(), this->node_public_key_, this->node_key_));
  return transactions::transaction_log::save(sp, t, data);
}

vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>
vds::dht::network::client::start_save(
  const service_provider * sp) const {

  GET_EXPECTED(original_file, file_stream_output_async::create_tmp(sp));
  GET_EXPECTED(original_hash, hash_stream_output_async::create(hash::sha256(), original_file));

  return original_hash;
}

vds::async_task<vds::expected<vds::dht::network::client::chunk_info>>
vds::dht::network::client::finish_save(
  const service_provider * sp,
  const std::shared_ptr<vds::stream_output_async<uint8_t>> stream) {
  
  auto original_hash = std::dynamic_pointer_cast<hash_stream_output_async>(stream);
  if(!original_hash) {
    co_return make_unexpected<vds::vds_exceptions::invalid_operation>();
  }

  auto original_file = std::dynamic_pointer_cast<file_stream_output_async>(original_hash->target());
  if (!original_file) {
    co_return make_unexpected<vds::vds_exceptions::invalid_operation>();
  }
  
  auto key_data = original_hash->signature();

  if (key_data.size() != symmetric_crypto::aes_256_cbc().key_size()
    || sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()) {
    co_return vds::make_unexpected<std::runtime_error>("Design error");
  }

  auto key = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    key_data.data(),
    pack_block_iv);

  auto null_steam = std::make_shared<null_stream_output_async>();
  GET_EXPECTED_ASYNC(crypto_hash, hash_stream_output_async::create(hash::sha256(), null_steam));
  GET_EXPECTED_ASYNC(crypto_stream, symmetric_encrypt::create(key, crypto_hash));

  CHECK_EXPECTED_ASYNC(original_file->target().seek(0));

  uint8_t buffer[16 * 1024];
  for (;;) {
    GET_EXPECTED_ASYNC(readed, original_file->target().read(buffer, sizeof(buffer)));
    CHECK_EXPECTED_ASYNC(co_await crypto_stream->write_async(buffer, readed));

    if (0 == readed) {
      break;
    }
  }

  auto key_data2 = crypto_hash->signature();

  auto key2 = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    key_data2.data(),
    pack_block_iv);


  GET_EXPECTED_ASYNC(save_stream, this->impl_->create_save_stream());
  GET_EXPECTED_ASYNC(crypto_stream2, symmetric_encrypt::create(key2, save_stream));
  GET_EXPECTED_ASYNC(deflate_steam2, deflate::create(crypto_stream2));

  CHECK_EXPECTED_ASYNC(original_file->target().seek(0));
  for (;;) {
    GET_EXPECTED_ASYNC(readed, original_file->target().read(buffer, sizeof(buffer)));
    CHECK_EXPECTED_ASYNC(co_await deflate_steam2->write_async(buffer, readed));

    if (0 == readed) {
      break;
    }
  }

  std::vector<vds::const_data_buffer> info;
  GET_EXPECTED_VALUE_ASYNC(info, co_await save_stream->save());

  co_return chunk_info
  {
    key_data,
    key_data2,
    info
  };
}


vds::async_task<vds::expected<vds::const_data_buffer>> vds::dht::network::client::restore(
  
  const chunk_info& block_id) {
  GET_EXPECTED_ASYNC(result, co_await this->impl_->restore(block_id.object_ids, std::chrono::steady_clock::now()));

  auto key2 = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    block_id.key.data(),
    pack_block_iv);

  GET_EXPECTED_ASYNC(zipped, symmetric_decrypt::decrypt(key2, result));
  GET_EXPECTED_ASYNC(original_data, inflate::decompress(zipped.data(), zipped.size()));

  GET_EXPECTED_ASYNC(sig, hash::signature(hash::sha256(), original_data));
  if(block_id.id != sig) {
    co_return make_unexpected<std::runtime_error>("Data is corrupted");
  }

  co_return original_data;
}

vds::async_task<vds::expected<vds::const_data_buffer>> vds::dht::network::client::restore(std::vector<const_data_buffer> object_ids)
{
  return this->impl_->restore(object_ids, std::chrono::steady_clock::now());
}

vds::expected<vds::dht::network::client::block_info_t> vds::dht::network::client::prepare_restore(
  database_read_transaction & t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const chunk_info& block_id) {
  return this->impl_->prepare_restore(t, final_tasks, block_id.object_ids);
}


const vds::const_data_buffer& vds::dht::network::client::current_node_id() const {
  return this->impl_->current_node_id();
}

void vds::dht::network::client::get_route_statistics(route_statistic& result) {
  this->impl_->get_route_statistics(result);
}

void vds::dht::network::client::get_session_statistics(session_statistic& session_statistic) {
  this->impl_->get_session_statistics(session_statistic);
}

void vds::dht::network::client::update_wellknown_connection_enabled(bool value) {
  this->impl_->update_wellknown_connection_enabled(value);
}
