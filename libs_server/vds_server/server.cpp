/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "private/server_p.h"
#include "user_manager.h"
#include "db_model.h"
#include "dht_network.h"
#include "transaction_log_hierarchy_dbo.h"
#include "chunk_dbo.h"
#include "chunk_replica_data_dbo.h"
#include "dht_object_id.h"
#include "../vds_dht_network/private/dht_network_client_p.h"
#include "../vds_log_sync/include/sync_process.h"
#include "sync_process.h"
#include "sync_state_dbo.h"
#include "sync_member_dbo.h"
#include "sync_replica_map_dbo.h"
#include "transaction_log_record_dbo.h"
#include "local_data_dbo.h"

vds::server::server()
: impl_(new _server(this))
{
}

vds::server::~server()
{
}

vds::expected<void> vds::server::register_services(service_registrator& registrator)
{
  registrator.add_service<server>(this);
  registrator.add_service<dht::network::imessage_map>(this->impl_.get());
  registrator.add_service<db_model>(this->impl_->db_model_.get());

  CHECK_EXPECTED(this->impl_->dht_network_service_->register_services(registrator));

  return expected<void>();
}

vds::expected<void> vds::server::start(const service_provider * sp)
{
  return this->impl_->start(sp);
}

vds::expected<void> vds::server::stop()
{
  return this->impl_->stop();
}

vds::async_task<vds::expected<void>> vds::server::start_network(
  uint16_t port,
  bool dev_network) {
  CHECK_EXPECTED_ASYNC(this->impl_->dht_network_service_->start(this->impl_->sp_, this->impl_->udp_transport_, port, dev_network));
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::server::prepare_to_stop() {
  return this->impl_->prepare_to_stop();
}

vds::async_task<vds::expected<vds::server_statistic>> vds::server::get_statistic() const {
  return this->impl_->get_statistic();
}

/////////////////////////////////////////////////////////////////////////////////////////////
vds::_server::_server(server * owner)
: owner_(owner),
  db_model_(new db_model()),
  udp_transport_(new dht::network::udp_transport()),
  dht_network_service_(new dht::network::service()),
  update_timer_("Log Sync") {
}

vds::_server::~_server()
{
}

vds::expected<void> vds::_server::start(const service_provider* sp)
{
  this->sp_ = sp;
  CHECK_EXPECTED(this->db_model_->start(sp));
  this->transaction_log_sync_process_.reset(new transaction_log::sync_process(sp));

  return this->update_timer_.start(sp, std::chrono::seconds(60), [sp, pthis = this->shared_from_this()]() -> async_task<expected<bool>>{
    std::list<std::function<async_task<expected<void>>()>> final_tasks;
    CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_transaction([pthis, &final_tasks](database_transaction & t) -> expected<void> {
        if (!pthis->sp_->get_shutdown_event().is_shuting_down()) {
          CHECK_EXPECTED(pthis->transaction_log_sync_process_->do_sync(t, final_tasks));
        }
        return expected<void>();
      }));

    while (!final_tasks.empty()) {
      CHECK_EXPECTED_ASYNC(co_await final_tasks.front()());
      final_tasks.pop_front();
    }

    co_return !sp->get_shutdown_event().is_shuting_down();
  });
}

vds::expected<void> vds::_server::stop()
{
  CHECK_EXPECTED(this->dht_network_service_->stop());
  CHECK_EXPECTED(this->db_model_->stop());
  this->udp_transport_.reset();
  this->db_model_.reset();

  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::_server::prepare_to_stop() {
  this->udp_transport_->stop();
  CHECK_EXPECTED_ASYNC(co_await this->dht_network_service_->prepare_to_stop());
  CHECK_EXPECTED_ASYNC(co_await this->db_model_->prepare_to_stop());
  co_return expected<void>();
}

vds::async_task<vds::expected<vds::server_statistic>> vds::_server::get_statistic() {
  auto result = std::make_shared<vds::server_statistic>();
  
  result->db_queue_length_ = this->sp_->get<db_model>()->queue_length();
  GET_EXPECTED_VALUE_ASYNC(result->current_user_, persistence::current_user(this->sp_));
  GET_EXPECTED_VALUE_ASYNC(result->local_machine_, persistence::local_machine(this->sp_));
  this->sp_->get<dht::network::client>()->get_route_statistics(result->route_statistic_);
  this->sp_->get<dht::network::client>()->get_session_statistics(result->session_statistic_);

  co_return *result;
}

vds::expected<bool> vds::_server::apply_message(
  database_transaction & t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const dht::messages::transaction_log_state & message,
  const message_info_t & message_info) {
  return this->transaction_log_sync_process_->apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::_server::apply_message(
  database_transaction & t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const dht::messages::transaction_log_request & message,
  const message_info_t & message_info) {
  return this->transaction_log_sync_process_->apply_message(t, final_tasks, message, message_info);
}

vds::expected<bool> vds::_server::apply_message(
  database_transaction & t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const dht::messages::transaction_log_record & message,
  const message_info_t & message_info) {
  return this->transaction_log_sync_process_->apply_message(t, final_tasks, message, message_info);
}

vds::async_task<vds::expected<void>> vds::_server::on_new_session( const_data_buffer partner_id) {
  
  std::list<std::function<async_task<expected<void>>()>> final_tasks;
  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_read_transaction([this, partner_id, &final_tasks](database_read_transaction & t) -> expected<void> {
    CHECK_EXPECTED(this->transaction_log_sync_process_->on_new_session(t, final_tasks, partner_id));
    CHECK_EXPECTED((*this->sp_->get<dht::network::client>())->on_new_session(t, final_tasks, partner_id));
    return expected<void>();
  }));

  while (!final_tasks.empty()) {
    CHECK_EXPECTED_ASYNC(co_await final_tasks.front()());
    final_tasks.pop_front();
  }

  co_return expected<void>();
}
