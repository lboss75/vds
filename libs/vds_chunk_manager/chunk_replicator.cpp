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
			THIS_MODULE,
			sp,
			"%s at storing replica",
			ex->what());
	});

}

void vds::_chunk_replicator::update_replicas(
    const service_provider &sp,
    database_transaction &t) {
/*
  asymmetric_private_key device_private_key;
  auto this_device = sp.get<user_manager>()->get_current_device(sp, device_private_key);
  auto p2p = sp.get<p2p_network>();

  p2p->get_neighbors();

  dbo::chunk_data_dbo t1;
  dbo::chunk_map_dbo t2;
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
	if (!block_id.empty()) {
		blocks.push_back(block_id);
	}
  }
  sp.get<logger>()->trace(THIS_MODULE, sp, "Found %d blocks to generate replicas", blocks.size());

  if(!blocks.empty()) {
      for (const auto & block_id : blocks) {
        std::set<int> replicas;

        st = t.get_reader(t2.select(t2.replica).where(t2.id == block_id));
        while (st.execute()) {
          auto replica = t2.replica.get(st);
          replicas.emplace(replica);
        }

		st = t.get_reader(t1.select(t1.block_data).where(t1.id == block_id));
		if (!st.execute()) {
			throw std::runtime_error("Unable to load block data");
		}

		resizable_data_buffer data;
      	data += t1.block_data.get(st);
		if (0 != (data.size() % (sizeof(uint16_t) * chunk_replicator::MIN_HORCRUX))) {
			data.padding(sizeof(uint16_t) * chunk_replicator::MIN_HORCRUX - (data.size() % (sizeof(uint16_t) * chunk_replicator::MIN_HORCRUX)));
		}

        uint16_t replica = 0;

        while(replicas.end() != replicas.find(replica)) {
			++replica;
        }

        if(replica > chunk_replicator::GENERATE_HORCRUX) {
			break;
        }

	      const auto replica_data = this->generate_replica(replica, data.data(), data.size());
          p2p->save_data(
              sp,
              this_device.id(),
			  cert_control::get_user_id(this_device.user_certificate()),
			  replica_data);
    }
  }*/
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

