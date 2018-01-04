/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_replicator.h"
#include "private/chunk_replicator_p.h"
#include "db_model.h"
#include "chunk_data_dbo.h"
#include "p2p_network.h"
#include "chunk_replica_dbo.h"
#include "chunk_map_dbo.h"
#include "chunk.h"
#include "messages/chunk_send_replica.h"
#include "member_user.h"
#include "user_manager.h"

void vds::chunk_replicator::start(const vds::service_provider &sp) {
  this->impl_.reset(new _chunk_replicator());
  this->impl_->start(sp);
}

void vds::chunk_replicator::stop(const vds::service_provider &sp) {
  this->impl_->stop(sp);
  this->impl_.reset();
}

/////////////////////////////////////////////////////////////////////
vds::_chunk_replicator::_chunk_replicator()
: update_timer_("Chunk Replicator"),
  in_update_timer_(false){

}

void vds::_chunk_replicator::start(const vds::service_provider &sp) {
  this->update_timer_.start(
      sp,
      std::chrono::seconds(5),
      [sp, pthis = this->shared_from_this()]()->bool{
        std::unique_lock<std::mutex> lock(pthis->update_timer_mutex_);
        if(!pthis->in_update_timer_){
          pthis->in_update_timer_ = true;
          sp.get<db_model>()->async_transaction(sp, [sp, pthis](database_transaction & t)->bool{
            try {
              pthis->update_replicas(sp, t);

              std::unique_lock<std::mutex> lock(pthis->update_timer_mutex_);
              pthis->in_update_timer_ = false;

              return true;
            }
            catch(...) {
              std::unique_lock<std::mutex> lock(pthis->update_timer_mutex_);
              pthis->in_update_timer_ = false;
              throw;
            }
          });
        }

        return !sp.get_shutdown_event().is_shuting_down();
      });
}

void vds::_chunk_replicator::stop(const vds::service_provider &sp) {
  this->update_timer_.stop(sp);

  for(;;) {
    std::unique_lock<std::mutex> lock(this->update_timer_mutex_);
    if (!this->in_update_timer_) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void vds::_chunk_replicator::update_replicas(
    const service_provider &sp,
    database_transaction &t) {

  asymmetric_private_key device_private_key;
  auto this_device = sp.get<user_manager>()->get_current_device(sp, t, device_private_key);

  chunk_data_dbo t1;
  chunk_map_dbo t2;
  auto st = t.get_reader(
      t1.select(t1.id, db_count(t2.replica))
          .left_join(t2, t1.id == t2.id)
          .order_by(db_count(t2.replica)));

  std::list<std::string> blocks;
  while(st.execute()){
    int replica_count;
    st.get_value(1, replica_count);

    if(chunk_replicator::GENERATE_HORCRUX <= replica_count){
      break;
    }

    auto block_id = t1.id.get(st);
    blocks.push_back(block_id);
  }
  sp.get<logger>()->trace(THIS_MODULE, sp, "Found %d blocks to generate replicas", blocks.size());

  if(!blocks.empty()) {
    auto p2p = sp.get<p2p_network>();
    auto nodes = p2p->active_nodes();
    sp.get<logger>()->trace(THIS_MODULE, sp, "Found %d nodes", nodes.size());
    if(!nodes.empty()) {
      for (auto block_id : blocks) {
        std::set<guid> candidates(nodes);
        std::set<int> replicas;

        st = t.get_reader(t2.select(t2.replica, t2.device).where(t2.id == block_id));
        while (st.execute()) {
          auto replica = t2.replica.get(st);
          replicas.emplace(replica);

          auto node_id = t2.device.get(st);
          auto p = candidates.find(node_id);
          if (candidates.end() != p) {
            candidates.erase(p);
            if(candidates.empty()){
              break;
            }
          }
        }

        const_data_buffer data;
        uint16_t replica = 0;
        for(auto node : candidates) {
          while(replicas.end() != replicas.find(replica)) {
            ++replica;
          }
          if(replica > chunk_replicator::GENERATE_HORCRUX) {
            break;
          }

          if(!data){
            st = t.get_reader(t1.select(t1.block_data).where(t1.id == block_id));
            if(!st.execute()){
              throw std::runtime_error("Unable to load block data");
            }
            data = t1.block_data.get(st);
          }

          p2p->send(
              sp,
              node,
              p2p_messages::chunk_send_replica(
                  this_device.id(),
                  this->generate_replica(replica, data)).serialize());
        }
      }
    }
  }
}

vds::const_data_buffer vds::_chunk_replicator::generate_replica(
    uint16_t replica,
    const vds::const_data_buffer &buffer) {

  chunk_generator<uint16_t> * generator;

  auto p = this->generators_.find(replica);
  if (this->generators_.end() == p) {
    generator = new chunk_generator<uint16_t>(chunk_replicator::MIN_HORCRUX, replica);
    this->generators_[replica].reset(generator);
  }
  else {
    generator = p->second.get();
  }

  binary_serializer s;
  generator->write(s, buffer.data(), buffer.size());

  return s.data();
}

