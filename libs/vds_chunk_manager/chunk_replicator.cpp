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
    dbo::chunk_replica_data_dbo t1;
    auto st = t.get_reader(t1.select(t1.id, t1.replica, t1.replica_data).where(t1.replica_hash == base64::from_bytes(message.data_hash())));
    if(st.execute()) {
      sp.get<p2p_network>()->send(
        sp,
        message.source_node_id(),
        p2p_messages::chunk_send_replica(
          base64::to_bytes(t1.id.get(st)),
          t1.replica.get(st),
          message.data_hash(),
          t1.replica_data.get(st)).serialize());
    }
    return true;
  }).execute([sp, message](const std::shared_ptr<std::exception> & ex) {
    if(ex) {
      sp.get<logger>()->warning(ThisModule, sp, "%s at query replica %s", ex->what(), base64::from_bytes(message.data_hash()).c_str());
    }
  });
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
	const p2p_messages::chunk_send_replica& message)
{
	auto sp = parent_scope.create_scope(__FUNCTION__);
	mt_service::enable_async(sp);

	sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this(), message](database_transaction & t)->bool {
		pthis->store_replica(sp, t, message);
		return true;
	}).execute([sp](const std::shared_ptr<std::exception> & ex) {
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

  auto p2p = sp.get<p2p_network>();
  auto neighbors = p2p->get_neighbors();

  auto user_mng = sp.get<user_manager>();

  for (auto & neighbor : neighbors) {

    dbo::chunk_replica_data_dbo t1;
    auto st = t.get_reader(t1.select(t1.replica_hash).order_by(db_desc_order(t1.distance)));
    while (st.execute()) {
      auto replica_hash = base64::to_bytes(t1.replica_hash.get(st));
      auto distance = p2p->calc_distance(neighbor.node_id, replica_hash);
      bool is_good = true;

      for (auto & user_sp : user_mng->current_users()) {
        auto this_device_id = user_sp.get_property<current_run_configuration>(service_provider::property_scope::any_scope)->id();
        if (p2p->calc_distance(this_device_id, replica_hash) < distance ) {
          is_good = false;
        }
      }

      if (is_good) {
        p2p->send(sp, neighbor.node_id, p2p_messages::chunk_offer_replica(replica_hash).serialize());
      }
    }
  }
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
	const ::vds::database_transaction& t,
	const p2p_messages::chunk_send_replica& message)
{
	throw std::runtime_error("Not implemented");
}

