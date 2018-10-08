/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "private/server_p.h"
#include "user_manager.h"
#include "db_model.h"
#include "file_manager_service.h"
#include "dht_network.h"
#include "transaction_log_unknown_record_dbo.h"
#include "chunk_dbo.h"
#include "chunk_replica_data_dbo.h"
#include "dht_object_id.h"
#include "../vds_dht_network/private/dht_network_client_p.h"
#include "../vds_log_sync/include/sync_process.h"
#include "sync_process.h"
#include "sync_state_dbo.h"
#include "sync_member_dbo.h"
#include "sync_replica_map_dbo.h"

vds::server::server()
: impl_(new _server(this))
{
}

vds::server::~server()
{
}

void vds::server::register_services(service_registrator& registrator)
{
  registrator.add_service<server>(this);
  registrator.add_service<dht::network::imessage_map>(this->impl_.get());
  registrator.add_service<db_model>(this->impl_->db_model_.get());

  this->impl_->dht_network_service_->register_services(registrator);
  this->impl_->file_manager_->register_services(registrator);
}

void vds::server::start(const service_provider * sp)
{
  this->impl_->start(sp);
}

void vds::server::stop()
{
  this->impl_->stop();
}

std::future<void> vds::server::start_network( uint16_t port) {
  this->impl_->dht_network_service_->start(this->impl_->sp_, this->impl_->udp_transport_, port);
  this->impl_->file_manager_->start(this->impl_->sp_);
  co_return;
}

std::future<void> vds::server::prepare_to_stop() {
  return this->impl_->prepare_to_stop();
}

std::future<vds::server_statistic> vds::server::get_statistic() const {
  return this->impl_->get_statistic();
}
/////////////////////////////////////////////////////////////////////////////////////////////
vds::_server::_server(server * owner)
: owner_(owner),
  db_model_(new db_model()),
  file_manager_(new file_manager::file_manager_service()),
  udp_transport_(new dht::network::udp_transport()),
  dht_network_service_(new dht::network::service()),
  update_timer_("Log Sync") {
}

vds::_server::~_server()
{
}

void vds::_server::start(const service_provider* sp)
{
  this->sp_ = sp;
  this->db_model_->start(sp);
  this->transaction_log_sync_process_.reset(new transaction_log::sync_process(sp));

  this->update_timer_.start(sp, std::chrono::seconds(60), [sp, pthis = this->shared_from_this()](){
    std::unique_lock<std::debug_mutex> lock(pthis->update_timer_mutex_);
    if (!pthis->in_update_timer_) {
      pthis->in_update_timer_ = true;
      lock.unlock();

      sp->get<db_model>()->async_transaction([pthis](database_transaction & t) {
        if (!pthis->sp_->get_shutdown_event().is_shuting_down()) {
          pthis->transaction_log_sync_process_->do_sync(t);
        }
      }).get();

      lock.lock();
      pthis->in_update_timer_ = false;
    }

    return !sp->get_shutdown_event().is_shuting_down();
  });
}

void vds::_server::stop()
{
  if (*this->file_manager_) {
    this->file_manager_->stop();
  }

  this->dht_network_service_->stop();
  this->udp_transport_->stop();
  this->db_model_->stop();
  this->file_manager_.reset();
  this->db_model_.reset();
}

std::future<void> vds::_server::prepare_to_stop() {
  co_await this->dht_network_service_->prepare_to_stop();
  co_await this->db_model_->prepare_to_stop();
}

std::future<vds::server_statistic> vds::_server::get_statistic() {
  auto result = std::make_shared<vds::server_statistic>();
  this->sp_->get<dht::network::client>()->get_route_statistics(result->route_statistic_);
  this->sp_->get<dht::network::client>()->get_session_statistics(result->session_statistic_);

  co_await this->sp_->get<db_model>()->async_read_transaction([this, result](database_read_transaction & t){

    orm::transaction_log_record_dbo t2;
    auto st = t.get_reader(t2.select(t2.id, t2.state, t2.order_no));

    while (st.execute()) {
      result->sync_statistic_.leafs_.push_back(
        sync_statistic::log_info_t {
          t2.id.get(st),
          static_cast<int>(t2.state.get(st)),
          t2.order_no.get(st)
        });
    }

    orm::transaction_log_unknown_record_dbo t3;
    st = t.get_reader(t3.select(t3.id));

    while(st.execute()) {
      auto id = t3.id.get(st);

      if(result->sync_statistic_.unknown_.end() == result->sync_statistic_.unknown_.find(id)) {
        result->sync_statistic_.unknown_.emplace(id);
      }
    }

    orm::chunk_dbo t4;
    st = t.get_reader(t4.select(t4.object_id));

    while (st.execute()) {
      auto id = t4.object_id.get(st);

      if (result->sync_statistic_.chunks_.end() == result->sync_statistic_.chunks_.find(id)) {
        result->sync_statistic_.chunks_.emplace(id);
      }
    }

    orm::chunk_replica_data_dbo t5;
    st = t.get_reader(t5.select(t5.object_id, t5.replica));

    while (st.execute()) {
      result->sync_statistic_.chunk_replicas_[t5.object_id.get(st)].emplace(t5.replica.get(st));
    }

    //Sync statistics
    const auto client = this->sp_->get<dht::network::client>();

    std::set<const_data_buffer> leaders;
    orm::sync_state_dbo t6;
    orm::sync_member_dbo t7;
    st = t.get_reader(
      t6.select(
        t6.object_id,
        t6.state,
        t7.generation,
        t7.current_term,
        t7.commit_index,
        t7.last_applied)
      .inner_join(t7, t7.object_id == t6.object_id && t7.member_node == client->current_node_id()));
    while(st.execute()) {
      std::string state;
      switch(t6.state.get(st)) {
      case orm::sync_state_dbo::state_t::canditate:
        state = 'c';
        break;
      case orm::sync_state_dbo::state_t::follower:
        state = 'f';
        break;
      case orm::sync_state_dbo::state_t::leader:
        state = 'l';
        leaders.emplace(t6.object_id.get(st));
        break;
      }
      result->sync_statistic_.sync_states_[t6.object_id.get(st)].node_state_ = state 
      + std::to_string(t7.generation.get(st))
      + "," + std::to_string(t7.current_term.get(st))
      + "," + std::to_string(t7.commit_index.get(st))
      + "," + std::to_string(t7.last_applied.get(st))
      ;
    }

    for(const auto & leader : leaders) {
      orm::sync_message_dbo t9;
      st = t.get_reader(t9.select(
        t9.index,
        t9.message_type,
        t9.member_node,
        t9.replica,
        t9.source_node,
        t9.source_index)
        .where(t9.object_id == leader));
      while (st.execute()) {
        std::string ch;
        switch(t9.message_type.get(st)) {
        case orm::sync_message_dbo::message_type_t::add_member:
          ch = "m+";
          break;
        case orm::sync_message_dbo::message_type_t::remove_replica:
          ch = "r-";
          break;
        case orm::sync_message_dbo::message_type_t::add_replica:
          ch = "r+";
          break;
        case orm::sync_message_dbo::message_type_t::remove_member:
          ch = "m-";
          break;

        }
        result->sync_statistic_.sync_states_[leader].messages_[t9.index.get(st)] = sync_statistic::sync_message {
          ch,
          t9.member_node.get(st),
          t9.replica.get(st),
          t9.source_node.get(st),
          t9.source_index.get(st)
        };
      }
    }

    st = t.get_reader(t7.select(t7.object_id, t7.member_node, t7.voted_for));
    while(st.execute()) {
      result->sync_statistic_.sync_states_[t7.object_id.get(st)].members_[t7.member_node.get(st)].voted_for_ = t7.voted_for.get(st);
    }

    orm::sync_replica_map_dbo t8;
    st = t.get_reader(t8.select(t8.object_id, t8.node, t8.replica));
    while (st.execute()) {
      result->sync_statistic_.sync_states_[t8.object_id.get(st)].members_[t8.node.get(st)].replicas_.emplace(t8.replica.get(st));
    }

  });

  co_return *result;
}

std::future<void> vds::_server::apply_message(
  
  database_transaction & t,
  const dht::messages::transaction_log_state & message,
  const message_info_t & message_info) {
  return this->transaction_log_sync_process_->apply_message(t, message, message_info);
}

void vds::_server::apply_message(
  
  database_transaction & t,
  const dht::messages::transaction_log_request & message,
  const message_info_t & message_info) {
  this->transaction_log_sync_process_->apply_message(t, message, message_info);
}

void vds::_server::apply_message(
  
  database_transaction & t,
  const dht::messages::transaction_log_record & message,
  const message_info_t & message_info) {
  this->transaction_log_sync_process_->apply_message(t, message, message_info);
}

std::future<void> vds::_server::on_new_session( const const_data_buffer& partner_id) {
  
  co_await this->sp_->get<db_model>()->async_read_transaction([this, partner_id](database_read_transaction & t) {

    this->transaction_log_sync_process_->on_new_session(t, partner_id);
    (*this->sp_->get<dht::network::client>())->on_new_session(t, partner_id);
  });
}
