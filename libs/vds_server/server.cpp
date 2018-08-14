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

void vds::server::start(const service_provider& sp)
{
  this->impl_->start(sp);
}

void vds::server::stop(const service_provider& sp)
{
  this->impl_->stop(sp);
}

vds::async_task<> vds::server::start_network(const vds::service_provider &sp, uint16_t port) {
  return [this, sp, port]() {
    this->impl_->dht_network_service_->start(sp, port);
    this->impl_->file_manager_->start(sp);
  };
}

vds::async_task<> vds::server::prepare_to_stop(const vds::service_provider &sp) {
  return this->impl_->prepare_to_stop(sp);
}

vds::async_task<vds::server_statistic> vds::server::get_statistic(const vds::service_provider &sp) const {
  return this->impl_->get_statistic(sp);
}

/////////////////////////////////////////////////////////////////////////////////////////////

vds::_server::_server(server * owner)
: owner_(owner),
  db_model_(new db_model()),
  file_manager_(new file_manager::file_manager_service()),
  dht_network_service_(new dht::network::service()),
  update_timer_("Log Sync") {
}

vds::_server::~_server()
{
}

void vds::_server::start(const service_provider& scope)
{
  this->db_model_->start(scope);
  this->transaction_log_sync_process_.reset(new transaction_log::sync_process());

  auto sp = scope.create_scope("Server Update Timer");
  imt_service::enable_async(sp);

  this->update_timer_.start(sp, std::chrono::seconds(1), [sp, pthis = this->shared_from_this()](){
    std::unique_lock<std::debug_mutex> lock(pthis->update_timer_mutex_);
    if (!pthis->in_update_timer_) {
      pthis->in_update_timer_ = true;
      lock.unlock();

      sp.get<db_model>()->async_transaction(sp, [sp, pthis](database_transaction & t) {
        if (!sp.get_shutdown_event().is_shuting_down()) {
          pthis->transaction_log_sync_process_->do_sync(sp, t);
        }
      }).execute([sp, pthis](const std::shared_ptr<std::exception> & ex) {
        if (ex) {
        }
        std::unique_lock<std::debug_mutex> lock(pthis->update_timer_mutex_);
        pthis->in_update_timer_ = false;
      });
    }

    return !sp.get_shutdown_event().is_shuting_down();
  });
}

void vds::_server::stop(const service_provider& sp)
{
  if (*this->file_manager_) {
    this->file_manager_->stop(sp);
  }

  this->dht_network_service_->stop(sp);
  this->db_model_->stop(sp);
  this->file_manager_.reset();
  this->db_model_.reset();
}

vds::async_task<> vds::_server::prepare_to_stop(const vds::service_provider &sp) {
  return async_series(
    this->dht_network_service_->prepare_to_stop(sp),
    this->db_model_->prepare_to_stop(sp)
  );
}

vds::async_task<vds::server_statistic> vds::_server::get_statistic(const vds::service_provider &sp) {
  auto result = std::make_shared<vds::server_statistic>();
  sp.get<dht::network::client>()->get_route_statistics(result->route_statistic_);
  sp.get<dht::network::client>()->get_session_statistics(result->session_statistic_);
  return sp.get<db_model>()->async_read_transaction(sp, [this, result](database_read_transaction & t){

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
  }).then([result]()->server_statistic{
    return *result;
  });

}

vds::async_task<> vds::_server::apply_message(
  const service_provider & sp,
  database_transaction & t,
  const dht::messages::transaction_log_state & message,
  const message_info_t & message_info) {
  return this->transaction_log_sync_process_->apply_message(sp, t, message, message_info);
}

void vds::_server::apply_message(
  const service_provider & sp,
  database_transaction & t,
  const dht::messages::transaction_log_request & message,
  const message_info_t & message_info) {
  this->transaction_log_sync_process_->apply_message(sp, t, message, message_info);
}

void vds::_server::apply_message(
  const service_provider & sp,
  database_transaction & t,
  const dht::messages::transaction_log_record & message,
  const message_info_t & message_info) {
  this->transaction_log_sync_process_->apply_message(sp, t, message, message_info);
}
