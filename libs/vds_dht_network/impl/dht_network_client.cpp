/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <db_model.h>
#include "stdafx.h"
#include "dht_network_client.h"
#include "private/dht_network_client_p.h"
#include "chunk_replica_data_dbo.h"
#include "messages/dht_find_node.h"

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

void vds::dht::network::_client::send(const service_provider& sp, const const_data_buffer& target_node_id,
  const message_type_t message_id, const const_data_buffer& message) {
  this->route_.for_near(
    sp,
    target_node_id,
    1,
    [sp, target_node_id, message_id, message, pthis = this->shared_from_this()](const const_data_buffer &node_id, const std::shared_ptr<dht_session> &proxy_session) {
    proxy_session->send_message(
      sp,
      pthis->udp_transport_,
      (uint8_t)message_id,
      message).wait();
    return false;
  });
}


void vds::dht::network::_client::start(const vds::service_provider &sp, uint16_t port) {
  this->udp_transport_ = std::make_shared<udp_transport>();
  this->udp_transport_->start(sp, port, this->current_node_id());

  this->update_timer_.start(sp, std::chrono::seconds(1), [sp, pthis = this->shared_from_this()](){
    if(!pthis->in_update_timer_){
      pthis->in_update_timer_ = true;
      sp.get<db_model>()->async_transaction(sp, [sp, pthis](database_transaction & t){
        pthis->process_update(sp, t);
        return true;
      }).execute([sp, pthis](const std::shared_ptr<std::exception> & ex){
        if(ex){
        }
        pthis->in_update_timer_ = false;
      });

    }

    return !sp.get_shutdown_event().is_shuting_down();
  });
}

void vds::dht::network::_client::stop(const service_provider& sp) {
  this->udp_transport_->stop(sp);
}

void vds::dht::network::_client::update_route_table(const service_provider& sp) {
  for (size_t i = 0; i < 8 * this->route_.current_node_id().size(); ++i) {
    auto canditate = dht_object_id::generate_random_id(this->route_.current_node_id(), i);

    this->send(
      sp,
      canditate,
      messages::dht_find_node(canditate, this->route_.current_node_id()));
  }
}

void vds::dht::network::_client::process_update(const vds::service_provider &sp, vds::database_transaction &t) {
  this->udp_transport_->on_timer(sp);
  this->route_.on_timer(sp);
  this->update_route_table(sp);
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
