/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "dht_network_client.h"
#include "private/dht_network_client_p.h"
#include "chunk_replica_data_dbo.h"
#include "messages/dht_find_node.h"
#include "messages/dht_find_node_response.h"
#include "messages/dht_ping.h"
#include "messages/dht_pong.h"
#include "deflate.h"
#include "db_model.h"
#include "async_task.h"

vds::dht::network::_client::_client(
    const service_provider & sp,
    const const_data_buffer & node_id)
: update_timer_("DHT Network"),
  in_update_timer_(false),
  route_(node_id)
{
  for (uint16_t replica = 0; replica < GENERATE_HORCRUX; ++replica) {
    this->generators_.emplace(replica, chunk_generator<uint16_t>(MIN_HORCRUX, replica));
  }
}

void vds::dht::network::_client::save(
    const service_provider &sp,
    database_transaction &t,
    const const_data_buffer & key,
    const const_data_buffer & value) {

  for(uint16_t replica = 0; replica < GENERATE_HORCRUX; ++replica) {
    chunk_generator<uint16_t> *generator;

    binary_serializer s;
    this->generators_.find(replica)->second.write(s, value.data(), value.size());
    const auto replica_data = s.data();

    orm::chunk_replica_data_dbo t1;
    t.execute(
        t1.insert(
            t1.id = base64::from_bytes(key),
            t1.replica = replica,
            t1.replica_data = replica_data
        ));
  }
}

vds::async_task<> vds::dht::network::_client::apply_message(const service_provider& sp, database_transaction& t,
  const messages::channel_log_state& message) {
  return this->sync_process_.apply_message(sp, t, message);
}

vds::async_task<> vds::dht::network::_client::apply_message(const service_provider& sp, database_transaction& t,
  const messages::channel_log_request& message) {
  return this->sync_process_.apply_message(sp, t, message);
}

void vds::dht::network::_client::apply_message(const service_provider& sp, database_transaction& t,
  const messages::channel_log_record& message) {
  this->sync_process_.apply_message(sp, t, message);
}

vds::async_task<> vds::dht::network::_client::apply_message(const service_provider& sp, const messages::dht_find_node& message) {
  std::map<const_data_buffer /*distance*/, std::list<std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>>> result_nodes;
  this->route_.search_nodes(sp, message.target_id(), 70, result_nodes);

  std::list<messages::dht_find_node_response::target_node> result;
  for (auto &presult : result_nodes) {
    for (auto & node : presult.second) {
      result.push_back(
        messages::dht_find_node_response::target_node(
          node->node_id_, node->proxy_session_->address().to_string(), node->hops_));
    }
  }

  return this->send(sp, message.source_node(), messages::dht_find_node_response(result));
}

vds::async_task<>  vds::dht::network::_client::apply_message(
  const service_provider& sp,
  const std::shared_ptr<dht_session> & session,
  const messages::dht_find_node_response& message) {
  auto result = async_task<>::empty();
  for(auto & p : message.nodes()) {
    this->route_.add_node(sp, p.target_id_, session, p.hops_ + 1);
    result = result.then([sp, pthis = this->shared_from_this(), address = p.address_]() {
      return pthis->udp_transport_->try_handshake(sp, address);
    });
  }
  return result;
}

vds::async_task<> vds::dht::network::_client::apply_message(const service_provider& sp, const std::shared_ptr<dht_session>& session,
  const messages::dht_ping& message) {
  if(message.target_node() == this->current_node_id()) {
    return session->send_message(
      sp,
      this->udp_transport_,
      (uint8_t)messages::dht_pong::message_id,
      messages::dht_pong(message.source_node(), this->current_node_id()).serialize());
  }
  else {
    auto result = async_task<>::empty();
    this->route_.for_near(sp, message.target_node(), 1, [this, &message, &result, sp](const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node> & candidate)->bool {
      if (dht_object_id::distance(message.target_node(), candidate->node_id_) < dht_object_id::distance(message.target_node(), this->current_node_id())) {
        result = this->send(sp, message.target_node(), message);
      }
      return true;
    });
    return result;
  }
}

vds::async_task<> vds::dht::network::_client::apply_message(const service_provider& sp, const std::shared_ptr<dht_session>& session,
  const messages::dht_pong& message) {
  this->route_.mark_pinged(message.target_node(), session->address());

  auto result = async_task<>::empty();
  this->route_.for_near(sp, message.target_node(), 1, [this, &message, &result, sp](const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node> & candidate)->bool {
    if (dht_object_id::distance(message.target_node(), candidate->node_id_) < dht_object_id::distance(message.target_node(), this->current_node_id())) {
      result = this->send(sp, message.target_node(), message);
    }
    return true;
  });
  return result;
}

void vds::dht::network::_client::add_session(const service_provider& sp, const std::shared_ptr<dht_session>& session, uint8_t hops) {
  this->route_.add_node(sp, session->partner_node_id(), session, hops);
}

vds::async_task<> vds::dht::network::_client::send(const service_provider& sp, const const_data_buffer& target_node_id,
  const message_type_t message_id, const const_data_buffer& message) {
  auto result = async_task<>::empty();
  this->route_.for_near(
    sp,
    target_node_id,
    1,
    [sp, target_node_id, message_id, message, &result, pthis = this->shared_from_this()](const std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node> & candidate) {
    result = candidate->send_message(
      sp,
      pthis->udp_transport_,
      message_id,
      message);
    return false;
  });
  return result;
}


void vds::dht::network::_client::start(const vds::service_provider &sp, uint16_t port) {
  this->udp_transport_ = std::make_shared<udp_transport>();
  this->udp_transport_->start(sp, port, this->current_node_id());

  this->update_timer_.start(sp, std::chrono::seconds(1), [sp, pthis = this->shared_from_this()](){
    std::unique_lock<std::debug_mutex> lock(pthis->update_timer_mutex_);
    if(!pthis->in_update_timer_){
      pthis->in_update_timer_ = true;
      lock.unlock();

      auto async_tasks = std::make_shared<async_task<>>(async_task<>::empty());
      sp.get<db_model>()->async_transaction(sp, [sp, pthis, async_tasks](database_transaction & t){
        *async_tasks = pthis->process_update(sp, t);
        return true;
      })
      .then([async_tasks]() {
        return std::move(*async_tasks);
      })
      .execute([sp, pthis](const std::shared_ptr<std::exception> & ex){
        if(ex){
        }
        std::unique_lock<std::debug_mutex> lock(pthis->update_timer_mutex_);
        pthis->in_update_timer_ = false;
      });

    }

    return !sp.get_shutdown_event().is_shuting_down();
  });
}

void vds::dht::network::_client::stop(const service_provider& sp) {
  this->udp_transport_->stop(sp);
}

vds::async_task<> vds::dht::network::_client::update_route_table(const service_provider& sp) {
  auto result = async_task<>::empty();
  for (size_t i = 0; i < 8 * this->route_.current_node_id().size(); ++i) {
    auto canditate = dht_object_id::generate_random_id(this->route_.current_node_id(), i);
    result = result.then([pthis = this->shared_from_this(), sp, canditate]() {
      return pthis->send(
        sp,
        canditate,
        messages::dht_find_node(canditate, pthis->route_.current_node_id()));
    });
  }
  return result;
}

vds::async_task<> vds::dht::network::_client::process_update(const vds::service_provider &sp, vds::database_transaction &t) {
  return async_series(
    this->route_.on_timer(sp, this->udp_transport_),
    this->update_route_table(sp),
    this->sync_process_.do_sync(sp, t));
}

void vds::dht::network::client::start(
  const vds::service_provider &sp,
  const vds::const_data_buffer &this_node_id, uint16_t port) {
  this->impl_.reset(new _client(sp, this_node_id));
  this->impl_->start(sp, port);

}

void vds::dht::network::client::stop(const service_provider& sp) {
  if(this->impl_) {
    this->impl_->stop(sp);
  }
}

vds::dht::network::client::chunk_info vds::dht::network::client::save(const service_provider& sp,
  database_transaction& t, const const_data_buffer& data) {

  static const uint8_t pack_block_iv[] = {
    // 0     1     2     3     4     5     6     7
    0xa5, 0xbb, 0x9f, 0xce, 0xc2, 0xe4, 0x4b, 0x91,
    0xa8, 0xc9, 0x59, 0x44, 0x62, 0x55, 0x90, 0x24
  };

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
  this->impl_->save(sp, t, key_data, crypted_data);
  return chunk_info
  {
    key_data,
    key_data2
  };
}
