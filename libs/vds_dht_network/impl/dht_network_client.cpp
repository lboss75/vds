/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <device_config_dbo.h>
#include <device_record_dbo.h>
#include "stdafx.h"
#include "dht_network_client.h"
#include "private/dht_network_client_p.h"
#include "chunk_dbo.h"
#include "messages/dht_find_node.h"
#include "messages/dht_find_node_response.h"
#include "messages/dht_ping.h"
#include "messages/dht_pong.h"
#include "messages/sync_replica_request.h"
#include "deflate.h"
#include "db_model.h"

#include "inflate.h"
#include "vds_exceptions.h"
#include "local_data_dbo.h"
#include "well_known_node_dbo.h"
#include "messages/sync_replica_data.h"
#include "dht_network.h"
#include "sync_replica_map_dbo.h"

bool vds::dht::network::client::is_debug = false;

vds::dht::network::_client::_client(
  const service_provider * sp,
  const std::shared_ptr<iudp_transport> & udp_transport,
  const std::shared_ptr<certificate> & node_cert,
  const std::shared_ptr<asymmetric_private_key> & node_key)
  : route_(node_cert->fingerprint(hash::sha256())),
    update_timer_("DHT Network"),
    update_route_table_counter_(0),
    udp_transport_(udp_transport){
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    this->generators_[replica].reset(new chunk_generator<uint16_t>(service::MIN_HORCRUX, replica));
  }

}

std::vector<vds::const_data_buffer> vds::dht::network::_client::save(
  const service_provider * sp,
  database_transaction& t,
  const const_data_buffer& value) {

  std::vector<const_data_buffer> result(service::GENERATE_HORCRUX);
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    binary_serializer s;
    this->generators_.find(replica)->second->write(s, value.data(), value.size());
    const auto replica_data = s.move_data();
    const auto replica_hash = hash::signature(hash::sha256(), replica_data);
    const auto& object_id = replica_hash;

    orm::chunk_dbo t1;
    orm::sync_replica_map_dbo t2;
    auto st = t.get_reader(t1.select(t1.object_id).where(t1.replica_hash == object_id));
    if (st.execute()) {
      if (t1.object_id.get(st) != object_id) {
        throw std::runtime_error("data conflict");
      }
    }
    else {
      auto client = sp->get<dht::network::client>();
      save_data(sp, t, replica_hash, replica_data);
      t.execute(
        t1.insert(
          t1.object_id = object_id,
          t1.replica_hash = object_id,
          t1.last_sync = std::chrono::system_clock::now() - std::chrono::hours(24)
        ));
      for (uint16_t distributed_replica = 0; distributed_replica < service::GENERATE_DISTRIBUTED_PIECES; ++distributed_replica) {
        t.execute(
          t2.insert(
            t2.object_id = object_id,
            t2.node = client->current_node_id(),
            t2.replica = distributed_replica,
            t2.last_access = std::chrono::system_clock::now()
          ));
      }

      this->sync_process_.add_sync_entry(sp, t, object_id, replica_data.size());
    }
    result[replica] = replica_hash;
  }

  return result;
}

void vds::dht::network::_client::apply_message(
  const service_provider * sp,
  const messages::dht_find_node& message,
  const imessage_map::message_info_t& message_info) {
  std::map<const_data_buffer /*distance*/, std::map<
             const_data_buffer, std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>>> result_nodes;
  this->route_.search_nodes(sp, message.target_id(), 70, result_nodes);

  std::list<messages::dht_find_node_response::target_node> result;
  for (auto& presult : result_nodes) {
    for (auto& node : presult.second) {
      result.emplace_back(
        node.second->node_id_,
        node.second->proxy_session_->address().to_string(),
        node.second->hops_);
    }
  }

  sp->get<logger>()->trace(ThisModule, sp, "Send dht_find_node_response");
  this->send(
    sp,
    message_info.source_node(),
    messages::dht_find_node_response(result));
}

std::future<void> vds::dht::network::_client::apply_message(
  const service_provider * sp,
  const messages::dht_find_node_response& message,
  const imessage_map::message_info_t& message_info) {
  for (auto& p : message.nodes()) {
    if (
      p.target_id_ != this->current_node_id()
      && this->route_.add_node(
      sp,
      p.target_id_,
      message_info.session(),
      p.hops_ + message_info.hops() + 1,
      true)) {
      co_await this->udp_transport_->try_handshake(sp, p.address_);
    }
  }
}

void vds::dht::network::_client::apply_message(
  const service_provider * sp,
  const messages::dht_ping& message,
  const imessage_map::message_info_t& message_info) {

  sp->get<logger>()->trace(ThisModule, sp, "Send dht_pong");
  message_info.session()->send_message(
    sp,
    this->udp_transport_,
    (uint8_t)messages::dht_pong::message_id,
    message_info.source_node(),
    messages::dht_pong().serialize());
}

void vds::dht::network::_client::apply_message(
  const service_provider * sp,
  const messages::dht_pong& message,
  const imessage_map::message_info_t& message_info) {
  this->route_.mark_pinged(
    message_info.source_node(),
    message_info.session()->address());
}

void vds::dht::network::_client::add_session(
  const service_provider * sp,
  const std::shared_ptr<dht_session>& session,
  uint8_t hops) {
  this->route_.add_node(sp, session->partner_node_id(), session, hops, false);
}

std::future<void> vds::dht::network::_client::send(
  const service_provider * sp,
  const const_data_buffer& target_node_id,
  const message_type_t message_id,
  const const_data_buffer& message) {
  co_await this->route_.for_near(
    sp,
    target_node_id,
    1,
    [sp, target_node_id, message_id, message, pthis = this->shared_from_this()](
    const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate) -> std::future<bool>{
      co_await candidate->proxy_session_->send_message(
        sp,
        pthis->udp_transport_,
        (uint8_t)message_id,
        target_node_id,
        message);
      co_return false;
    });
}

std::future<void> vds::dht::network::_client::send_near(
  const service_provider * sp,
  const const_data_buffer& target_node_id,
  size_t radius,
  const message_type_t message_id,
  const const_data_buffer& message) {
  co_await this->route_.for_near(
    sp,
    target_node_id,
    radius,
    [sp, target_node_id, message_id, message, this](
    const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate) -> std::future<bool> {

      co_await candidate->proxy_session_->send_message(
        sp,
        this->udp_transport_,
        (uint8_t)message_id,
        candidate->node_id_,
        message);
      co_return true;
    });
}

std::future<void> vds::dht::network::_client::send_near(
  const service_provider * sp,
  const const_data_buffer& target_node_id,
  size_t radius,
  const message_type_t message_id,
  const const_data_buffer& message,
  const std::function<bool(const dht_route<std::shared_ptr<dht_session>>::node& node)>& filter) {

  co_await this->route_.for_near(
    sp,
    target_node_id,
    radius,
    filter,
    [sp, target_node_id, message_id, message, this](
    const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate) -> std::future<bool> {
      co_await candidate->proxy_session_->send_message(
        sp,
        this->udp_transport_,
        (uint8_t)message_id,
        candidate->node_id_,
        message);
      co_return true;
    });
}

std::future<void> vds::dht::network::_client::proxy_message(
    const service_provider *sp,
    const const_data_buffer &target_node_id,
    message_type_t message_id,
    const const_data_buffer &message,
    const const_data_buffer &source_node,
    uint16_t hops) {
  co_await this->route_.for_near(
    sp,
    target_node_id,
    1,
    [
      sp,
      target_node_id,
      message_id,
      message,
      pthis = this->shared_from_this(),
      distance = dht_object_id::distance(this->current_node_id(), target_node_id),
      source_node,
      hops](
    const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate)->std::future<bool> {
      if (dht_object_id::distance(candidate->node_id_, target_node_id) < distance
        && source_node != candidate->proxy_session_->partner_node_id()) {

        sp->get<logger>()->trace(
          "dht_protocol",
          sp,
          "%s Message %d from %s to %s redirected to %s over %s",
          base64::from_bytes(pthis->current_node_id()).c_str(),
          message_id,
          base64::from_bytes(source_node).c_str(),
          base64::from_bytes(target_node_id).c_str(),
          base64::from_bytes(candidate->node_id_).c_str(),
          candidate->proxy_session_->address().to_string().c_str());

        co_await candidate->proxy_session_->proxy_message(
          sp,
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

std::future<void> vds::dht::network::_client::send_neighbors(const service_provider * sp,
                                                const message_type_t message_id, const const_data_buffer& message) {
  co_await this->route_.for_neighbors(
    sp,
    [sp, message_id, message, pthis = this->shared_from_this()](
      const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>& candidate)->std::future <bool> {
      co_await candidate->proxy_session_->send_message(
        sp,
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


void vds::dht::network::_client::start(const service_provider * sp) {
  this->update_timer_.start(sp, std::chrono::seconds(60), [sp, pthis = this->shared_from_this()]() {
    std::unique_lock<std::debug_mutex> lock(pthis->update_timer_mutex_);
    if (!pthis->in_update_timer_) {
      pthis->in_update_timer_ = true;
      lock.unlock();

      sp->get<db_model>()->async_transaction(sp, [sp, pthis](database_transaction& t) {
        pthis->process_update(sp, t).get();
        return true;
      }).get();
      
      std::unique_lock<std::debug_mutex> lock(pthis->update_timer_mutex_);
      pthis->in_update_timer_ = false;
    }

    return !sp->get_shutdown_event().is_shuting_down();
  });
}

void vds::dht::network::_client::stop(const service_provider * sp) {
  //this->udp_transport_->stop(sp);
}

void vds::dht::network::_client::get_neighbors(const service_provider * sp,
                                               std::list<std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>>
                                               & result) {
  this->route_.get_neighbors(sp, result);
}

void vds::dht::network::_client::on_new_session(
  const service_provider * sp,
  database_read_transaction& t,
  const const_data_buffer& partner_id) {
  this->sync_process_.on_new_session(
    sp,
    t,
    partner_id);
}

void vds::dht::network::_client::remove_session(
  const service_provider * sp,
  const std::shared_ptr<dht_session>& session) {
  this->route_.remove_session(sp, session);
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
  db_value<uint64_t> data_size;
  auto st = t.get_reader(
    t3.select(t3.local_path, t3.reserved_size, db_sum(t4.data_size).as(data_size))
      .left_join(t4, t4.node_id == t3.node_id && t4.storage_path == t3.local_path)
      .where(t3.node_id == client->current_node_id())
      .group_by(t3.local_path, t3.reserved_size));
  while (st.execute()) {
    const uint64_t size = data_size.is_null(st) ? 0 : data_size.get(st);
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

std::future<void> vds::dht::network::_client::update_route_table(const service_provider * sp) {
  if (0 == this->update_route_table_counter_) {
    for (size_t i = 0; i < 8 * this->route_.current_node_id().size(); ++i) {
      auto canditate = dht_object_id::generate_random_id(this->route_.current_node_id(), i);
      co_await this->send_neighbors(
        sp,
        messages::dht_find_node(canditate));

    }
    this->update_route_table_counter_ = 100;
  }
  else {
    this->update_route_table_counter_--;
  }
}

std::future<void> vds::dht::network::_client::process_update(const service_provider * sp, database_transaction& t) {
  this->sync_process_.do_sync(sp, t);

  co_await this->route_.on_timer(sp, this->udp_transport_);
  co_await this->update_route_table(sp);
  co_await this->update_wellknown_connection(sp, t);
}

void vds::dht::network::_client::get_route_statistics(route_statistic& result) {
  this->route_.get_statistics(result);
}

void vds::dht::network::_client::get_session_statistics(session_statistic& session_statistic) {
  static_cast<udp_transport *>(this->udp_transport_.get())->get_session_statistics(session_statistic);
}

void vds::dht::network::_client::apply_message(
  const service_provider * sp,
  database_transaction& t,
  const messages::sync_new_election_request& message,
  const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(
  const service_provider * sp,
  database_transaction& t,
  const messages::sync_new_election_response& message,
  const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_add_message_request& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_leader_broadcast_request& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_leader_broadcast_response& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_replica_operations_request& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_replica_operations_response& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_looking_storage_request& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_looking_storage_response& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_snapshot_request& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_snapshot_response& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_offer_send_replica_operation_request& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_offer_remove_replica_operation_request& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
                                               const messages::sync_replica_request& message,
                                               const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(
  const service_provider * sp, database_transaction& t,
  const messages::sync_replica_data& message,
  const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

void vds::dht::network::_client::apply_message(const service_provider * sp, database_transaction& t,
  const messages::sync_replica_query_operations_request& message, const imessage_map::message_info_t& message_info) {
  this->sync_process_.apply_message(sp, t, message, message_info);
}

std::future<void> vds::dht::network::_client::restore(
  const service_provider * sp,
  const std::vector<const_data_buffer>& object_ids,
  const std::shared_ptr<const_data_buffer>& result,
  const std::chrono::steady_clock::time_point& start) {
  for (;;) {
    uint8_t progress = co_await this->restore_async(
      sp,
      object_ids,
      result);

    if (result->size() > 0) {
      co_return;
    }

    if (std::chrono::minutes(10) < (std::chrono::steady_clock::now() - start)) {
      throw vds_exceptions::not_found();
    }

    std::this_thread::sleep_for(std::chrono::minutes(2));
  }
}

std::future<uint8_t> vds::dht::network::_client::restore_async(
  const service_provider * sp,
  const std::vector<const_data_buffer>& object_ids,
  const std::shared_ptr<const_data_buffer>& result) {

  auto result_progress = std::make_shared<uint8_t>();
  co_await sp->get<db_model>()->async_transaction(
    sp,
    [pthis = this->shared_from_this(), sp, object_ids, result, result_progress](
      database_transaction& t) -> bool {

    std::vector<uint16_t> replicas;
    std::vector<const_data_buffer> datas;
    std::list<const_data_buffer> unknonw_replicas;

    orm::chunk_dbo t1;
    orm::device_record_dbo t4;
    for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
      auto st = t.get_reader(
        t1
        .select(t4.local_path)
        .inner_join(t4, t4.node_id == pthis->current_node_id() && t4.data_hash == t1.replica_hash)
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
      binary_serializer s;
      restore.restore(s, datas);
      *result = s.move_data();
      *result_progress = 100;
      return true;
    }

    *result_progress = 99 * replicas.size() / service::MIN_HORCRUX;
    for (const auto& replica : unknonw_replicas) {
      pthis->sync_process_.restore_replica(sp, t, replica);
    }
    return true;
  });

  co_return *result_progress;
}

std::future<void>
vds::dht::network::_client::update_wellknown_connection(
  const service_provider * sp,
  database_transaction& t) {

  orm::well_known_node_dbo t1;
  auto st = t.get_reader(t1.select(t1.addresses));
  while (st.execute()) {
    for (const auto& address : split_string(t1.addresses.get(st), ';', true)) {
      co_await this->udp_transport_->try_handshake(sp, address);
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
  const service_provider * sp,
  const const_data_buffer& source_node,
  uint16_t hops,
  const std::shared_ptr<dht_session>& session) {
  this->route_.add_node(sp, source_node, session, hops, false);

}

void vds::dht::network::_client::find_nodes(
    const vds::service_provider *sp,
    const vds::const_data_buffer &node_id,
    size_t radius) {

  this->send_neighbors(
      sp,
      messages::dht_find_node(node_id));
}

void vds::dht::network::client::start(
  const service_provider * sp,
  const std::shared_ptr<certificate> & node_cert,
  const std::shared_ptr<asymmetric_private_key> & node_key,
  const std::shared_ptr<iudp_transport> & udp_transport) {
  this->impl_.reset(new _client(sp, udp_transport, node_cert, node_key));
  this->impl_->start(sp);

}

void vds::dht::network::client::stop(const service_provider * sp) {
  if (this->impl_) {
    this->impl_->stop(sp);
  }
}

static const uint8_t pack_block_iv[] = {
  // 0     1     2     3     4     5     6     7
  0xa5, 0xbb, 0x9f, 0xce, 0xc2, 0xe4, 0x4b, 0x91,
  0xa8, 0xc9, 0x59, 0x44, 0x62, 0x55, 0x90, 0x24
};

vds::dht::network::client::chunk_info vds::dht::network::client::save(
  const service_provider * sp,
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

  auto zipped = deflate::compress(sp, data);

  auto crypted_data = symmetric_encrypt::encrypt(key2, zipped);
  return chunk_info
  {
    key_data,
    key_data2,
    this->impl_->save(sp, t, crypted_data)
  };
}

std::future<vds::const_data_buffer> vds::dht::network::client::restore(
  const service_provider * sp,
  const chunk_info& block_id) {
  auto result = std::make_shared<const_data_buffer>();
  co_await this->impl_->restore(sp, block_id.object_ids, result, std::chrono::steady_clock::now());

  auto key2 = symmetric_key::create(
    symmetric_crypto::aes_256_cbc(),
    block_id.key.data(),
    pack_block_iv);

  auto zipped = symmetric_decrypt::decrypt(key2, *result);
  auto original_data = inflate::decompress(sp, zipped.data(), zipped.size());

  vds_assert(block_id.id == hash::signature(hash::sha256(), original_data));

  co_return original_data;

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
