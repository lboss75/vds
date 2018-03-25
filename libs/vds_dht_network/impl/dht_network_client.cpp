/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <db_model.h>
#include "stdafx.h"
#include "dht_network_client.h"
#include "private/dht_network_client_p.h"
#include "chunk_replica_data_dbo.h"

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
    this->generators_.find(replica).write(s, value.data(), value.size());
    auto replica_data = s.data();

    orm::chunk_replica_data_dbo t1;
    t.execute(
        t1.insert(
            t1.id = base64::from_bytes(key),
            t1.replica = replica,
            t1.replica_data = replica_data,
            t1.replica_hash = hash::signature(hash::sha256(), replica_data)
        ));
  }
}

void vds::dht::network::_client::start(const vds::service_provider &sp) {
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

void vds::dht::network::_client::process_update(const vds::service_provider &sp, vds::database_transaction &t) {
  this->route_.on_timer(sp);
}

void vds::dht::network::client::start(
    const vds::service_provider &sp,
    const vds::const_data_buffer &this_node_id) {
  this->impl_.reset(new _client(sp, this_node_id));

}
