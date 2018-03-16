/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
/*
#include <messages/chunk_have_replica.h>
#include "stdafx.h"
#include "chunk_replicator.h"
#include "private/chunk_replicator_p.h"
#include "db_model.h"
#include "chunk_data_dbo.h"
#include "p2p_network.h"
#include "chunk_replica_data_dbo.h"
#include "chunk.h"
#include "messages/chunk_send_replica.h"
#include "member_user.h"
#include "user_manager.h"
#include "cert_control.h"
#include "run_configuration_dbo.h"
#include "messages/chunk_offer_replica.h"

void vds::chunk_replicator::start(const vds::service_provider &sp) {
  this->impl_.reset(new _chunk_replicator());
  this->impl_->start(sp);
}

void vds::chunk_replicator::stop(const vds::service_provider &sp) {
  this->impl_->stop(sp);
  this->impl_.reset();
}

void vds::chunk_replicator::apply(const service_provider& sp, const guid& partner_id,
	const p2p_messages::chunk_send_replica& message)
{
	this->impl_->apply(sp, partner_id, message);
}

void vds::chunk_replicator::apply(
  const service_provider& sp,
  const guid& partner_id,
  const p2p_messages::chunk_query_replica& message) {
  sp.get<db_model>()->async_transaction(sp, [sp, message](database_transaction & t)->bool{
    orm::chunk_replica_data_dbo t1;
    auto st = t.get_reader(
        t1.select(t1.id, t1.replica, t1.replica_data)
            .where(t1.id == base64::from_bytes(message.object_id())
                   && db_not_in_values(t1.replica, message.exist_replicas())));
    if(st.execute()) {
      sp.get<p2p_network>()->send(
        sp,
        message.source_node_id(),
        p2p_messages::chunk_send_replica(
          message.object_id(),
          t1.replica.get(st),
          t1.replica_data.get(st),
          hash::signature(hash::sha256(), t1.replica_data.get(st))).serialize());
    }
    return true;
  }).execute([sp, message](const std::shared_ptr<std::exception> & ex) {
    if(ex) {
      sp.get<logger>()->warning(ThisModule, sp, "%s at query replica %s", ex->what(), base64::from_bytes(
          message.object_id()).c_str());
    }
  });
}

void vds::chunk_replicator::apply(
    const vds::service_provider &sp,
    const vds::guid &partner_id,
    const vds::p2p_messages::chunk_offer_replica &message) {

  sp.get<db_model>()->async_transaction(sp, [sp, message](database_transaction & t)->bool{
    auto p2p = sp.get<p2p_network>();
    std::set<uint16_t> exists;
    std::set<uint16_t> dublicates;
    orm::chunk_replica_data_dbo t1;
    auto st = t.get_reader(
        t1.select(t1.replica)
            .where(t1.id == base64::from_bytes(message.object_id())));
    if (st.execute()) {
      const auto replica = t1.replica.get(st);
      exists.emplace(replica);
      if(message.replicas().end() != message.replicas().find(replica)) {
        dublicates.emplace(replica);
      }
    }

    if (!dublicates.empty()) {
      p2p->send(
          sp,
          message.source_node_id(),
          p2p_messages::chunk_have_replica(
              p2p->current_node_id(),
              message.object_id(),
              dublicates).serialize());
    }

    if(exists.size() < message.replicas().size() - dublicates.size()) {
      p2p->send(
          sp,
          message.source_node_id(),
          p2p_messages::chunk_query_replica(
            p2p->current_node_id(),
              message.object_id(),
            exists).serialize());
    }
    return true;

  }).execute([sp, message](const std::shared_ptr<std::exception> & ex) {
    if(ex) {
      sp.get<logger>()->warning(ThisModule, sp, "%s at query replica %s", ex->what(), base64::from_bytes(message.object_id()).c_str());
    }
  });
}

void vds::chunk_replicator::apply(
    const vds::service_provider &sp,
    const vds::guid &partner_id,
    const vds::p2p_messages::chunk_have_replica &message) {
  //sp.get<db_model>()->async_transaction(sp, [sp, message](database_transaction & t)->bool{

  //  sp.get<logger>()->info(
  //      ThisModule,
  //      sp,
  //      "Remove replica %s because exist on node %s",
  //      base64::from_bytes(message.data_hash()).c_str(),
  //      message.node_id().str().c_str());

  //  dbo::chunk_replica_data_dbo t1;
  //  t.execute(t1.delete_if(t1.replica_hash == base64::from_bytes(message.data_hash())));
  //  return true;
  //}).execute([sp, message](const std::shared_ptr<std::exception> & ex) {
  //  if(ex) {
  //    sp.get<logger>()->warning(
  //        ThisModule,
  //        sp,
  //        "%s at removing replica %s",
  //        ex->what(),
  //        base64::from_bytes(message.data_hash()).c_str());
  //  }
  //});
}

vds::const_data_buffer vds::chunk_replicator::get_object(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::const_data_buffer &object_id) const {

  std::vector<uint16_t> replicas;
  std::vector<const_data_buffer> datas;

  orm::chunk_replica_data_dbo t1;
  auto st = t.get_reader(
      t1
          .select(t1.replica, t1.replica_data)
          .where(t1.id == base64::from_bytes(object_id)));

  while (st.execute()) {
    auto replica = safe_cast<uint16_t>(t1.replica.get(st));
    replicas.push_back(replica);

    auto replica_data = t1.replica_data.get(st);
    datas.push_back(replica_data);

    if (replicas.size() >= chunk_replicator::MIN_HORCRUX) {
      break;
    }
  }

  if (replicas.size() >= chunk_replicator::MIN_HORCRUX) {

    chunk_restore<uint16_t> restore(chunk_replicator::MIN_HORCRUX, replicas.data());
    binary_serializer s;
    restore.restore(s, datas);

    return s.data();
  } else {
    sp.get<logger>()->trace(
        ThisModule,
        sp,
        "Send query for replicas of %s",
        base64::from_bytes(object_id).c_str());
    std::set<uint16_t> qreplicas;
    for(auto p : replicas) {
      qreplicas.emplace(p);
    }
    auto p2p = sp.get<p2p_network>();
    p2p->query_replica(
        sp,
        object_id,
        qreplicas,
        chunk_replicator::GENERATE_HORCRUX);

    return const_data_buffer();
  }
}

void vds::chunk_replicator::put_object(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::const_data_buffer &object_id,
    const vds::const_data_buffer &data) {

  this->impl_->put_object(sp, t, object_id, data);
}

/////////////////////////////////////////////////////////////////////
vds::_chunk_replicator::_chunk_replicator()
: update_timer_("Chunk Replicator"),
  in_update_timer_(false){

}

void vds::_chunk_replicator::start(const vds::service_provider &parent_scope) {
	auto sp = parent_scope.create_scope(__FUNCTION__);
	mt_service::enable_async(sp);
  this->update_timer_.start(
      sp,
      std::chrono::seconds(5),
      [sp, pthis = this->shared_from_this()]()->bool{
        std::unique_lock<std::mutex> lock(pthis->update_timer_mutex_);
        if(!pthis->in_update_timer_){
          pthis->in_update_timer_ = true;
          sp.get<db_model>()->async_transaction(sp, [sp, pthis](database_transaction & t)->bool{
              pthis->update_replicas(sp, t);
              return true;
          }).execute([pthis](const std::shared_ptr<std::exception> & ex){
			  std::unique_lock<std::mutex> lock(pthis->update_timer_mutex_);
			  pthis->in_update_timer_ = false;
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

void vds::_chunk_replicator::apply(
	const service_provider& parent_scope,
	const guid& partner_id,
	const p2p_messages::chunk_send_replica& message) {
  auto sp = parent_scope.create_scope(__FUNCTION__);
  mt_service::enable_async(sp);

  sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this(), message](
      database_transaction &t) -> bool {
    pthis->store_replica(sp, t, message);
    return true;
  }).execute([sp](const std::shared_ptr<std::exception> &ex) {
    sp.get<logger>()->warning(
        ThisModule,
        sp,
        "%s at storing replica",
        ex->what());
  });
}

void vds::_chunk_replicator::update_replicas(
    const service_provider &sp,
    database_transaction &t) {

//  auto user_mng = sp.get<user_manager>();
//    auto p2p = sp.get<p2p_network>();
//    auto neighbors = p2p->get_neighbors();
//    for (auto &neighbor : neighbors) {
//      dbo::chunk_replica_data_dbo t1;
//      auto st = t.get_reader(t1.select(t1.replica_hash).order_by(db_desc_order(t1.distance)));
//      while (st.execute()) {
//        auto replica_hash = base64::to_bytes(t1.replica_hash.get(st));
//        auto this_distance = p2p_network::calc_distance(this_device_id, replica_hash);
//        auto distance = p2p->calc_distance(neighbor.node_id, replica_hash);
//        if (this_distance > distance) {
//          p2p->send(sp, neighbor.node_id, p2p_messages::chunk_offer_replica(
//              this_distance,
//              this_device_id,
//              replica_hash).serialize());
//        }
//      }
//    }
//  }
}

vds::const_data_buffer vds::_chunk_replicator::generate_replica(
    uint16_t replica,
	const void * data, size_t size) {

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
  generator->write(s, data, size);

  return s.data();
}

void vds::_chunk_replicator::store_replica(
	const service_provider& sp,
	database_transaction& t,
	const p2p_messages::chunk_send_replica& message)
{
	throw std::runtime_error("Not implemented");
}

void vds::_chunk_replicator::put_object(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::const_data_buffer &object_id,
    const vds::const_data_buffer &data) {

  for(uint16_t replica = 0; replica < chunk_replicator::GENERATE_HORCRUX; ++replica) {
    chunk_generator<uint16_t> *generator;

    std::unique_lock<std::mutex> lock(this->generators_mutex_);
    auto p = this->generators_.find(replica);
    if (this->generators_.end() == p) {
      generator = new chunk_generator<uint16_t>(chunk_replicator::MIN_HORCRUX, replica);
      this->generators_[replica].reset(generator);
    } else {
      generator = p->second.get();
    }
    lock.unlock();

    binary_serializer s;
    generator->write(s, data.data(), data.size());
    auto replica_data = s.data();

    orm::chunk_replica_data_dbo t1;
    t.execute(
        t1.insert(
            t1.id = base64::from_bytes(object_id),
            t1.replica = replica,
            t1.replica_data = replica_data,
            t1.replica_hash = hash::signature(hash::sha256(), replica_data)
        ));
  }
}

*/