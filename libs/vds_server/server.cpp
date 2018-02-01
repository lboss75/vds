/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "private/server_p.h"
#include "node_manager.h"
#include "user_manager.h"
#include "private/server_http_api_p.h"
#include "private/node_manager_p.h"
#include "private/storage_log_p.h"
#include "private/chunk_manager_p.h"
#include "private/server_database_p.h"
#include "private/local_cache_p.h"
#include "transaction_context.h"
#include "p2p_network.h"
#include "transaction_log.h"
#include "db_model.h"
#include "p2p_network_client.h"
#include "chunk_manager.h"
#include "private/p2p_network_p.h"
#include "log_sync_service.h"
#include "file_manager_service.h"
#include "chunk_replicator.h"

vds::server::server()
: impl_(new _server(this))
{
}

vds::server::~server()
{
  delete impl_;
}



void vds::server::register_services(service_registrator& registrator)
{
  registrator.add_service<iserver>(this->impl_);
  
  registrator.add_service<istorage_log>(this->impl_->storage_log_.get());

  registrator.add_service<principal_manager>(&(this->impl_->storage_log_->principal_manager_));
  
  registrator.add_service<chunk_manager>(this->impl_->chunk_manager_.get());
  
  registrator.add_service<iserver_database>(this->impl_->server_database_.get());
  
  registrator.add_service<ilocal_cache>(this->impl_->local_cache_.get());

  registrator.add_service<node_manager>(this->impl_->node_manager_.get());

  registrator.add_service<user_manager>(this->impl_->user_manager_.get());

  registrator.add_service<db_model>(this->impl_->db_model_.get());

  registrator.add_service<ip2p_network_client>(this->impl_->network_client_.get());

  registrator.add_service<p2p_network>(this->impl_->p2p_network_.get());

  registrator.add_service<log_sync_service>(this->impl_->log_sync_service_.get());

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

vds::async_task<vds::user_invitation> vds::server::reset(const vds::service_provider &sp, const std::string &root_user_name, const std::string &root_password,
                                     const std::string &device_name, int port) {
	auto result = std::make_shared<user_invitation>();
  return sp.get<db_model>()->async_transaction(sp, [this, sp, root_user_name, root_password, device_name, port, result](
      database_transaction & t){
    auto usr_manager = sp.get<user_manager>();
    auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
    *result = usr_manager->reset(sp, t, root_user_name, root_password, private_key, device_name, port);
	return true;
  }).then([result]() {
	  return *result;
	  
  });
}

vds::async_task<> vds::server::init_server(
	const vds::service_provider &sp,
	const user_invitation & request,
	const std::string & user_password,
	const std::string &device_name,
	int port) {
  return this->impl_->user_manager_->init_server(
      sp, request, user_password, device_name, port);
}

vds::async_task<> vds::server::start_network(const vds::service_provider &sp) {
  return this->impl_->p2p_network_->start_network(sp).then([this, sp]() {
    this->impl_->log_sync_service_->start(sp);
    this->impl_->file_manager_->start(sp);
    this->impl_->chunk_replicator_->start(sp);
  });
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
  node_manager_(new _node_manager()),
  user_manager_(new user_manager()),
  db_model_(new db_model()),
  server_http_api_(new _server_http_api()),
  storage_log_(new _storage_log()),
  chunk_manager_(new chunk_manager()),
  server_database_(new _server_database()),
  local_cache_(new _local_cache()),
  p2p_network_(new p2p_network()),
  network_client_(new p2p_network_client()),
  log_sync_service_(new log_sync_service()),
  file_manager_(new file_manager::file_manager_service()),
  chunk_replicator_(new chunk_replicator())

{
  this->leak_detect_.name_ = "server";
  this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector){
    collector->add(*this->p2p_network_);
    //collector->add(this->network_client_);
  };
}

vds::_server::~_server()
{
}

void vds::_server::start(const service_provider& sp)
{
	this->db_model_->start(sp);
}

void vds::_server::stop(const service_provider& sp)
{
  if (*this->chunk_replicator_) {
    this->chunk_replicator_->stop(sp);
  }

  if (*this->file_manager_) {
    this->file_manager_->stop(sp);
  }
  if (*this->log_sync_service_) {
    this->log_sync_service_->stop(sp);
  }

  this->db_model_->stop(sp);
  this->p2p_network_->stop(sp);

  this->chunk_replicator_.reset();
  this->file_manager_.reset();
  this->log_sync_service_.reset();
  this->db_model_.reset();
  this->p2p_network_.reset();
  this->network_client_.reset();
}

vds::async_task<> vds::_server::prepare_to_stop(const vds::service_provider &sp) {
  return async_series(
    this->log_sync_service_->prepare_to_stop(sp),
    this->db_model_->prepare_to_stop(sp),
    this->p2p_network_->prepare_to_stop(sp)
  );
}

vds::async_task<vds::server_statistic> vds::_server::get_statistic(const vds::service_provider &sp) {
  auto result = std::make_shared<vds::server_statistic>();
  return sp.get<db_model>()->async_transaction(sp, [this, result](database_transaction & t){
    this->log_sync_service_->get_statistic(t, result->sync_statistic_);
    return true;
  }).then([result]()->server_statistic{
    return *result;
  });

}
