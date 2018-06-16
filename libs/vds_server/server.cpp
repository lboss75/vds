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
#include "chunk_replicas_dbo.h"
#include "chunk_replica_data_dbo.h"

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
  sp.get<dht::network::client>()->get_route_statistics(result->route_statistic_);
  sp.get<dht::network::client>()->get_session_statistics(result->session_statistic_);
  return sp.get<db_model>()->async_transaction(sp, [this, result](database_transaction & t){

    orm::transaction_log_record_dbo t2;
    auto st = t.get_reader(t2.select(t2.id, t2.state, t2.order_no));

    while (st.execute()) {
      result->sync_statistic_.leafs_.push_back(
        sync_statistic::log_info_t {
          base64::to_bytes(t2.id.get(st)),
          t2.state.get(st),
          t2.order_no.get(st)
        });
    }

    orm::transaction_log_unknown_record_dbo t3;
    st = t.get_reader(t3.select(t3.id));

    while(st.execute()) {
      auto id = base64::to_bytes(t3.id.get(st));

      if(result->sync_statistic_.unknown_.end() == result->sync_statistic_.unknown_.find(id)) {
        result->sync_statistic_.unknown_.emplace(id);
      }
    }

    orm::chunk_replicas_dbo t4;
    st = t.get_reader(t4.select(t4.id));

    while (st.execute()) {
      auto id = base64::to_bytes(t4.id.get(st));

      if (result->sync_statistic_.replicas_.end() == result->sync_statistic_.replicas_.find(id)) {
        result->sync_statistic_.replicas_.emplace(id);
      }
    }

    orm::chunk_replica_data_dbo t5;
    st = t.get_reader(t5.select(t5.id, t5.replica));

    while (st.execute()) {
      const auto id = t5.id.get(st) + "." + std::to_string(t5.replica.get(st));

      if (result->sync_statistic_.replica_distribution_.end() == result->sync_statistic_.replica_distribution_.find(id)) {
        result->sync_statistic_.replica_distribution_.emplace(id);
      }
    }

    return true;
  }).then([result]()->server_statistic{
    return *result;
  });

}

