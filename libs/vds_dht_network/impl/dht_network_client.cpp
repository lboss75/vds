/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <device_config_dbo.h>
#include <device_record_dbo.h>
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

vds::dht::network::_client::_client(
  const service_provider * sp,
  const std::shared_ptr<iudp_transport> & udp_transport,
  const std::shared_ptr<certificate> & node_cert,
  const std::shared_ptr<asymmetric_private_key> & /*node_key*/)
  : sp_(sp),
    route_(sp, node_cert->fingerprint(hash::sha256())),
    update_timer_("DHT Network"),
    update_route_table_counter_(0),
    udp_transport_(udp_transport),
    sync_process_(sp),
  update_wellknown_connection_enabled_(true) {
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    this->generators_[replica].reset(new chunk_generator<uint16_t>(service::MIN_HORCRUX, replica));
  }

}

vds::async_task<std::vector<vds::const_data_buffer>> vds::dht::network::_client::save(
  database_transaction& t,
  const const_data_buffer& value) {

  std::vector<const_data_buffer> result(service::GENERATE_HORCRUX);
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    binary_serializer s;
    this->generators_.find(replica)->second->write(s, value.data(), value.size());
    const auto replica_data = s.move_data();
    const auto replica_hash = hash::signature(hash::sha256(), replica_data);

    orm::chunk_dbo t1;
    orm::sync_replica_map_dbo t2;
    auto st = t.get_reader(t1.select(t1.object_id).where(t1.object_id == replica_hash));
    if (!st.execute()) {
      auto client = this->sp_->get<dht::network::client>();
      save_data(this->sp_, t, replica_hash, replica_data);
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

      co_await this->sync_process_.add_sync_entry(t, replica_hash, replica_data.size());
    }
    result[replica] = replica_hash;
  }

  co_return result;
}


std::shared_ptr<vds::dht::network::client_save_stream> vds::dht::network::_client::create_save_stream() {
  return std::make_shared<client_save_stream>(this->sp_, this->generators_);
}

vds::async_task<void> vds::dht::network::_client::apply_message(
  
  const messages::dht_find_node& message,
  const imessage_map::message_info_t& message_info) {
  std::map<const_data_buffer /*distance*/,
  std::map<const_data_buffer, std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>>> result_nodes;
  this->route_.search_nodes(message.target_id, 70, result_nodes);

  std::list<messages::dht_find_node_response::target_node> result;
  for (auto& presult : result_nodes) {
    for (auto& node : presult.second) {
      result.emplace_back(
        node.second->node_id_,
        node.second->proxy_session_->address().to_string(),
        node.second->hops_);
    }
  }

  this->sp_->get<logger>()->trace(ThisModule, "Send dht_find_node_response");
  return this->send(
    message_info.source_node(),
    message_create<messages::dht_find_node_response>(result));
}

vds::async_task<void> vds::dht::network::_client::apply_message(
  
  const messages::dht_find_node_response& message,
  const imessage_map::message_info_t& message_info) {
  for (auto& p : message.nodes) {
    if (
      p.target_id_ != this->current_node_id()
      && this->route_.add_node(
      p.target_id_,
      message_info.session(),
      p.hops_ + message_info.hops() + 1,
      true)) {
      co_await this->udp_transport_->try_handshake(p.address_);
    }
  }
}

vds::async_task<void> vds::dht::network::_client::apply_message(
  
  const messages::dht_ping& message,
  const imessage_map::message_info_t& message_info) {

  this->sp_->get<logger>()->trace(ThisModule, "Send dht_pong");
  co_await message_info.session()->send_message(
    this->udp_transport_,
    (uint8_t)messages::dht_pong::message_id,
    message_info.source_node(),
    message_serialize(messages::dht_pong()));
}

vds::async_task<void> vds::dht::network::_client::apply_message(
  
  const messages::dht_pong& message,
  const imessage_map::message_info_t& message_info) {
  this->route_.mark_pinged(
    message_info.source_node(),
    message_info.session()->address());
  co_return;
}

vds::async_task<void> vds::dht::network::_client::add_session(
  const std::shared_ptr<dht_session>& session,
  uint8_t hops) {
  this->route_.add_node(session->partner_node_id(), session, hops, false);
  return this->sp_->get<db_model>()->async_transaction([address = session->address().to_string()](database_transaction& t) {
    orm::well_known_node_dbo t1;
    auto st = t.get_reader(t1.select(t1.last_connect).where(t1.address == address));
    if(st.execute()) {
      t.execute(t1.update(t1.last_connect = std::chrono::system_clock::now()).where(t1.address == address));
    }
    else {
      t.execute(t1.insert(t1.last_connect = std::chrono::system_clock::now(), t1.address = address));
    }
  });
}

vds::async_task<void> vds::dht::network::_client::send(
  
  const const_data_buffer& target_node_id,
  const message_type_t message_id,
  const const_data_buffer& message) {
  co_await this->route_.for_near(
    target_node_id,
    1,
    [target_node_id, message_id, message, pthis = this->shared_from_this()](
    const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate) -> async_task<bool>{
      co_await candidate->proxy_session_->send_message(
        pthis->udp_transport_,
        (uint8_t)message_id,
        target_node_id,
        message);
      co_return false;
    });
}

vds::async_task<void> vds::dht::network::_client::send_near(
  
  const const_data_buffer& target_node_id,
  size_t radius,
  const message_type_t message_id,
  const const_data_buffer& message) {
  co_await this->route_.for_near(
    target_node_id,
    radius,
    [target_node_id, message_id, message, this](
    const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate) -> vds::async_task<bool> {

      co_await candidate->proxy_session_->send_message(
        this->udp_transport_,
        (uint8_t)message_id,
        candidate->node_id_,
        message);
      co_return true;
    });
}

vds::async_task<void> vds::dht::network::_client::send_near(
  
  const const_data_buffer& target_node_id,
  size_t radius,
  const message_type_t message_id,
  const const_data_buffer& message,
  const std::function<bool(const dht_route<std::shared_ptr<dht_session>>::node& node)>& filter) {

  co_await this->route_.for_near(
    target_node_id,
    radius,
    filter,
    [target_node_id, message_id, message, this](
    const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate) -> vds::async_task<bool> {
      co_await candidate->proxy_session_->send_message(
        this->udp_transport_,
        (uint8_t)message_id,
        candidate->node_id_,
        message);
      co_return true;
    });
}

vds::async_task<void> vds::dht::network::_client::proxy_message(
    
    const const_data_buffer &target_node_id,
    message_type_t message_id,
    const const_data_buffer &message,
    const const_data_buffer &source_node,
    uint16_t hops) {
  co_await this->route_.for_near(
    target_node_id,
    1,
    [
      target_node_id,
      message_id,
      message,
      pthis = this->shared_from_this(),
      distance = dht_object_id::distance(this->current_node_id(), target_node_id),
      source_node,
      hops](
    const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate)->vds::async_task<bool> {
      if (dht_object_id::distance(candidate->node_id_, target_node_id) < distance
        && source_node != candidate->proxy_session_->partner_node_id()) {

        pthis->sp_->get<logger>()->trace(
          "dht_protocol",
          "%s Message %d from %s to %s redirected to %s over %s",
          base64::from_bytes(pthis->current_node_id()).c_str(),
          message_id,
          base64::from_bytes(source_node).c_str(),
          base64::from_bytes(target_node_id).c_str(),
          base64::from_bytes(candidate->node_id_).c_str(),
          candidate->proxy_session_->address().to_string().c_str());

        co_await candidate->proxy_session_->proxy_message(
          pthis->udp_transport_,
          (uint8_t)message_id,
          target_node_id,
          source_node,
          hops,
          message);
        co_return false;
      }
      co_return true;
    });
}

vds::async_task<void> vds::dht::network::_client::send_neighbors(
                                                const message_type_t message_id, const const_data_buffer& message) {
  co_await this->route_.for_neighbors(
    [message_id, message, pthis = this->shared_from_this()](
      const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate)->vds::async_task<bool> {
      co_await candidate->proxy_session_->send_message(
        pthis->udp_transport_,
        (uint8_t)message_id,
        candidate->node_id_,
        message);
      co_return true;
    });
}

vds::const_data_buffer vds::dht::network::_client::replica_id(const std::string& key, uint16_t replica) {
  auto id = "{" + std::to_string(replica) + "}" + key;
  return hash::signature(hash::sha256(), id.c_str(), id.length());
}


void vds::dht::network::_client::start() {
  this->update_timer_.start(this->sp_, std::chrono::seconds(60), [pthis = this->shared_from_this()]() -> async_task<bool>{

    co_await pthis->udp_transport_->on_timer();
      co_await pthis->sp_->get<db_model>()->async_transaction([pthis](database_transaction& t) {
        pthis->process_update(t).get();
        return true;
      });

      co_return !pthis->sp_->get_shutdown_event().is_shuting_down();
  });
}

void vds::dht::network::_client::stop() {
  //this->udp_transport_->stop(sp);
}

void vds::dht::network::_client::get_neighbors(
                                               std::list<std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>>
                                               & result) {
  this->route_.get_neighbors(result);
}

vds::async_task<void> vds::dht::network::_client::on_new_session(
  
  database_read_transaction& t,
  const const_data_buffer& partner_id) {
  return this->sync_process_.on_new_session(
    t,
    partner_id);
}

void vds::dht::network::_client::remove_session(
  
  const std::shared_ptr<dht_session>& session) {
  this->route_.remove_session(session);
}

vds::filename vds::dht::network::_client::save_data(
  const service_provider * sp,
  database_transaction& t,
  const const_data_buffer& data_hash,
  const const_data_buffer& data) {

  auto client = sp->get<network::client>();

  uint64_t allowed_size = 0;
  std::string local_path;

  orm::device_config_dbo t3;
  orm::device_record_dbo t4;
  db_value<int64_t> data_size;
  auto st = t.get_reader(
    t3.select(t3.local_path, t3.reserved_size, db_sum(t4.data_size).as(data_size))
      .left_join(t4, t4.node_id == t3.node_id && t4.storage_path == t3.local_path)
      .where(t3.node_id == client->current_node_id())
      .group_by(t3.local_path, t3.reserved_size));
  while (st.execute()) {
    const int64_t size = data_size.is_null(st) ? 0 : data_size.get(st);
    if (t3.reserved_size.get(st) > size && allowed_size < (t3.reserved_size.get(st) - size)) {
      allowed_size = (t3.reserved_size.get(st) - size);
      local_path = t3.local_path.get(st);
    }
  }

  if (local_path.empty() || allowed_size < data.size()) {
    throw std::runtime_error("No disk space");
  }

  auto append_path = base64::from_bytes(data_hash);
  str_replace(append_path, '+', '#');
  str_replace(append_path, '/', '_');

  foldername fl(local_path);
  fl.create();

  fl = foldername(fl, append_path.substr(0, 10));
  fl.create();

  fl = foldername(fl, append_path.substr(10, 10));
  fl.create();

  filename fn(fl, append_path.substr(20));
  file::write_all(fn, data);

  t.execute(t4.insert(
    t4.node_id = client->current_node_id(),
    t4.storage_path = local_path,
    t4.local_path = fn.full_name(),
    t4.data_hash = data_hash,
    t4.data_size = data.size()));

  return fn;
}

vds::filename vds::dht::network::_client::save_data(const service_provider* sp, database_transaction& t,
  const const_data_buffer& data_hash, const filename& original_file) {
  auto client = sp->get<network::client>();

  uint64_t allowed_size = 0;
  std::string local_path;

  orm::device_config_dbo t3;
  orm::device_record_dbo t4;
  db_value<int64_t> data_size;
  auto st = t.get_reader(
    t3.select(t3.local_path, t3.reserved_size, db_sum(t4.data_size).as(data_size))
    .left_join(t4, t4.node_id == t3.node_id && t4.storage_path == t3.local_path)
    .where(t3.node_id == client->current_node_id())
    .group_by(t3.local_path, t3.reserved_size));
  while (st.execute()) {
    const int64_t size = data_size.is_null(st) ? 0 : data_size.get(st);
    if (t3.reserved_size.get(st) > size && allowed_size < (t3.reserved_size.get(st) - size)) {
      allowed_size = (t3.reserved_size.get(st) - size);
      local_path = t3.local_path.get(st);
    }
  }
  auto size = file::length(original_file);
  if (local_path.empty() || allowed_size < size) {
    throw std::runtime_error("No disk space");
  }

  auto append_path = base64::from_bytes(data_hash);
  str_replace(append_path, '+', '#');
  str_replace(append_path, '/', '_');

  foldername fl(local_path);
  fl.create();

  fl = foldername(fl, append_path.substr(0, 10));
  fl.create();

  fl = foldername(fl, append_path.substr(10, 10));
  fl.create();

  filename fn(fl, append_path.substr(20));
  file::move(original_file, fn);

  t.execute(t4.insert(
    t4.node_id = client->current_node_id(),
    t4.storage_path = local_path,
    t4.local_path = fn.full_name(),
    t4.data_hash = data_hash,
    t4.data_size = size));

  return fn;
}

vds::async_task<void> vds::dht::network::_client::update_route_table() {
  if (0 == this->update_route_table_counter_) {
    for (size_t i = 0; i < 8 * this->route_.current_node_id().size(); ++i) {
      auto canditate = dht_object_id::generate_random_id(this->route_.current_node_id(), i);
      co_await this->send_neighbors(
        message_create<messages::dht_find_node>(std::move(canditate)));

    }
    this->update_route_table_counter_ = 10;
  }
  else {
    this->update_route_table_counter_--;
  }
}

vds::async_task<void> vds::dht::network::_client::process_update( database_transaction& t) {
  co_await this->sync_process_.do_sync(t);

  co_await this->route_.on_timer(this->udp_transport_);
  co_await this->update_route_table();
  co_await this->update_wellknown_connection(t);
}

void vds::dht::network::_client::get_route_statistics(route_statistic& result) {
  this->route_.get_statistics(result);
}

void vds::dht::network::_client::get_session_statistics(session_statistic& session_statistic) {
  static_cast<udp_transport *>(this->udp_transport_.get())->get_session_statistics(session_statistic);
}

vds::async_task<void> vds::dht::network::_client::apply_message(
  
  database_transaction& t,
  const messages::sync_new_election_request& message,
  const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message(
  
  database_transaction& t,
  const messages::sync_new_election_response& message,
  const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_add_message_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_leader_broadcast_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_leader_broadcast_response& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_replica_operations_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_replica_operations_response& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_looking_storage_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_looking_storage_response& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_snapshot_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_snapshot_response& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_offer_send_replica_operation_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_offer_remove_replica_operation_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
                                               const messages::sync_replica_request& message,
                                               const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message(
   database_transaction& t,
  const messages::sync_replica_data& message,
  const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::apply_message( database_transaction& t,
  const messages::sync_replica_query_operations_request& message, const imessage_map::message_info_t& message_info) {
  return this->sync_process_.apply_message(t, message, message_info);
}

vds::async_task<void> vds::dht::network::_client::restore(  
  const std::vector<const_data_buffer>& object_ids,
  const std::shared_ptr<const_data_buffer>& result,
  const std::chrono::steady_clock::time_point& start) {
  for (;;) {
    uint8_t progress = co_await this->restore_async(
      object_ids,
      result);

    if (result->size() > 0) {
      co_return;
    }

    if (std::chrono::minutes(10) < (std::chrono::steady_clock::now() - start)) {
      throw vds_exceptions::not_found();
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
}

vds::async_task<vds::dht::network::client::block_info_t> vds::dht::network::_client::prepare_restore(
  database_read_transaction & t,
  const std::vector<const_data_buffer>& object_ids) {

  auto result = vds::dht::network::client::block_info_t();

    for (const auto& object_id : object_ids) {
      result.replicas[object_id] = co_await this->sync_process_.prepare_restore_replica(t, object_id);
    }

  co_return result;
}

vds::async_task<uint8_t> vds::dht::network::_client::restore_async(
  
  const std::vector<const_data_buffer>& object_ids,
  const std::shared_ptr<const_data_buffer>& result) {

  auto result_progress = std::make_shared<uint8_t>();
  co_await this->sp_->get<db_model>()->async_transaction(
    [pthis = this->shared_from_this(), object_ids, result, result_progress](
      database_transaction& t) {

    std::vector<uint16_t> replicas;
    std::vector<const_data_buffer> datas;
    std::list<const_data_buffer> unknonw_replicas;

    orm::chunk_dbo t1;
    orm::device_record_dbo t4;
    for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
      auto st = t.get_reader(
        t1
        .select(t4.local_path)
        .inner_join(t4, t4.node_id == pthis->current_node_id() && t4.data_hash == t1.object_id)
        .where(t1.object_id == object_ids[replica]));

      if (st.execute()) {
        replicas.push_back(replica);
        datas.push_back(file::read_all(filename(t4.local_path.get(st))));


        if (replicas.size() >= service::MIN_HORCRUX) {
          break;
        }
      }
      else {
        unknonw_replicas.push_back(object_ids[replica]);
      }
    }

    if (replicas.size() >= service::MIN_HORCRUX) {
      chunk_restore<uint16_t> restore(service::MIN_HORCRUX, replicas.data());
      *result = restore.restore(datas);
      *result_progress = 100;
      return;
    }

    *result_progress = 99 * replicas.size() / service::MIN_HORCRUX;
    for (const auto& replica : unknonw_replicas) {
      pthis->sync_process_.restore_replica(t, replica).get();
    }
  });

  co_return *result_progress;
}

vds::async_task<void>
vds::dht::network::_client::update_wellknown_connection(
  database_transaction& t) {
  if (this->update_wellknown_connection_enabled_) {
    orm::well_known_node_dbo t1;
    auto st = t.get_reader(t1.select(t1.address));
    while (st.execute()) {
      auto address = t1.address.get(st);
      try {
        co_await this->udp_transport_->try_handshake(address);
      }
      catch (const std::system_error & ex) {
        this->sp_->get<logger>()->debug(ThisModule, "%s at handshake to %s", ex.what(), address.c_str());
      }
    }
  }
  else {
    try {
      co_await this->udp_transport_->try_handshake("udp://localhost:8050");
    }
    catch (const std::system_error & ex) {
    }
  }
}

vds::const_data_buffer
vds::dht::network::_client::read_data(
  const const_data_buffer& data_hash,
  const filename& data_path) {
  auto data = file::read_all(data_path);
  vds_assert(data_hash == hash::signature(hash::sha256(), data));
  return data;
}

void vds::dht::network::_client::delete_data(
  const const_data_buffer& /*replica_hash*/,
  const filename& data_path) {
  file::delete_file(data_path);
}

void vds::dht::network::_client::add_route(
  
  const const_data_buffer& source_node,
  uint16_t hops,
  const std::shared_ptr<dht_session>& session) {
  this->route_.add_node(source_node, session, hops, false);

}

vds::async_task<void> vds::dht::network::_client::find_nodes(
    
    const vds::const_data_buffer &node_id,
    size_t radius) {

  co_return co_await this->send_neighbors(message_create<messages::dht_find_node>(node_id));
}

void vds::dht::network::client::start(
  const service_provider * sp,
  const std::shared_ptr<certificate> & node_cert,
  const std::shared_ptr<asymmetric_private_key> & node_key,
  const std::shared_ptr<iudp_transport> & udp_transport) {
  this->impl_.reset(new _client(sp, udp_transport, node_cert, node_key));
  this->impl_->start();

}

void vds::dht::network::client::stop() {
  if (this->impl_) {
    this->impl_->stop();
  }
}

static const uint8_t pack_block_iv[] = {
  // 0     1     2     3     4     5     6     7
  0xa5, 0xbb, 0x9f, 0xce, 0xc2, 0xe4, 0x4b, 0x91,
  0xa8, 0xc9, 0x59, 0x44, 0x62, 0x55, 0x90, 0x24
};

vds::async_task<vds::dht::network::client::chunk_info> vds::dht::network::client::save(
  
  database_transaction& t,
  const const_data_buffer& data) {

  auto key_data = hash::signature(hash::sha256(), data);

  if (key_data.size() != symmetric_crypto::aes_256_cbc().key_size()
    || sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()) {
    throw std::runtime_error("Design error");
  }

  auto key = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    key_data.data(),
    pack_block_iv);

  auto key_data2 = hash::signature(
    hash::sha256(),
    symmetric_encrypt::encrypt(key, data));

  auto key2 = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    key_data2.data(),
    pack_block_iv);

  auto zipped = deflate::compress(data);

  auto crypted_data = symmetric_encrypt::encrypt(key2, zipped);
  auto info = co_await this->impl_->save(t, crypted_data);
  co_return chunk_info
  {
    key_data,
    key_data2,
    info
  };
}


vds::async_task<vds::dht::network::client::chunk_info> vds::dht::network::client::start_save(
  const service_provider * sp,
  const std::function<async_task<void>(const std::shared_ptr<stream_output_async<uint8_t>>& stream)>& data_writer) {

  auto tmp_file = std::make_shared<file>(file::create_temp(sp));
  auto original_file = std::make_shared<file_stream_output_async>(tmp_file.get());
  auto original_hash = std::make_shared<hash_stream_output_async>(hash::sha256(), original_file);

  co_await data_writer(original_hash);

  auto key_data = original_hash->signature();

  if (key_data.size() != symmetric_crypto::aes_256_cbc().key_size()
    || sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()) {
    throw std::runtime_error("Design error");
  }

  auto key = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    key_data.data(),
    pack_block_iv);

  auto null_steam = std::make_shared<null_stream_output_async>();
  auto crypto_hash = std::make_shared<hash_stream_output_async>(hash::sha256(), null_steam);
  auto crypto_stream = std::make_shared<symmetric_encrypt>(key, crypto_hash);

  tmp_file->seek(0);

  uint8_t buffer[16 * 1024];
  for(;;) {
    const auto readed = tmp_file->read(buffer, sizeof(buffer));
    co_await crypto_stream->write_async(buffer, readed);

    if(0 == readed) {
      break;
    }
  }

  auto key_data2 = crypto_hash->signature();

  auto key2 = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    key_data2.data(),
    pack_block_iv);


  auto save_stream = this->impl_->create_save_stream();
  crypto_stream = std::make_shared<symmetric_encrypt>(key2, save_stream);
  auto deflate_steam = std::make_shared<deflate>(crypto_stream);

  tmp_file->seek(0);
  for (;;) {
    const auto readed = tmp_file->read(buffer, sizeof(buffer));
    co_await deflate_steam->write_async(buffer, readed);

    if (0 == readed) {
      break;
    }
  }
  
  auto info = co_await save_stream->save();

  co_return chunk_info
  {
    key_data,
    key_data2,
    info
  };
}

vds::async_task<vds::const_data_buffer> vds::dht::network::client::restore(
  
  const chunk_info& block_id) {
  auto result = std::make_shared<const_data_buffer>();
  co_await this->impl_->restore(block_id.object_ids, result, std::chrono::steady_clock::now());

  auto key2 = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    block_id.key.data(),
    pack_block_iv);

  auto zipped = symmetric_decrypt::decrypt(key2, *result);
  auto original_data = inflate::decompress(zipped.data(), zipped.size());

  vds_assert(block_id.id == hash::signature(hash::sha256(), original_data));

  co_return original_data;

}

vds::async_task<vds::dht::network::client::block_info_t> vds::dht::network::client::prepare_restore(
  database_read_transaction & t,
  const chunk_info& block_id) {
  return this->impl_->prepare_restore(t, block_id.object_ids);
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
