/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "private/server_p.h"
#include "user_manager.h"
#include "private/chunk_manager_p.h"
#include "transaction_context.h"
#include "transaction_log.h"
#include "db_model.h"
#include "chunk_manager.h"
#include "log_sync_service.h"
#include "file_manager_service.h"
#include "chunk_replicator.h"
#include "dht_network.h"

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

vds::async_task<> vds::server::reset(
  const vds::service_provider &sp,
  const std::string &root_user_name,
  const std::string &root_password) {
  return sp.get<db_model>()->async_transaction(sp, [this, sp, root_user_name, root_password](
      database_transaction & t){
    auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());

    user_manager usr_manager;
    usr_manager.reset(sp, t, root_user_name, root_password, private_key);
	  return true;
  });
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
  dht_network_service_(new dht::network::service()){
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
  return sp.get<db_model>()->async_transaction(sp, [this, result](database_transaction & t){

    orm::transaction_log_record_dbo t2;
    auto st = t.get_reader(
        t2.select(t2.channel_id, t2.id)
            .where(t2.state == (int)orm::transaction_log_record_dbo::state_t::leaf));

    while (st.execute()) {
      result->sync_statistic_.leafs_[base64::to_bytes(t2.channel_id.get(st))].push_back(base64::to_bytes(t2.id.get(st)));
    }

    return true;
  }).then([result]()->server_statistic{
    return *result;
  });

}

