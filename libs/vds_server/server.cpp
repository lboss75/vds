/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "private/server_p.h"
#include "node_manager.h"
#include "user_manager.h"
#include "cert_manager.h"
#include "server_http_api.h"
#include "private/server_http_api_p.h"
#include "server_connection.h"
#include "server_udp_api.h"
#include "node_manager.h"
#include "private/node_manager_p.h"
#include "private/storage_log_p.h"
#include "private/chunk_manager_p.h"
#include "private/server_database_p.h"
#include "private/local_cache_p.h"
#include "private/node_manager_p.h"
#include "private/cert_manager_p.h"
#include "private/server_connection_p.h"
#include "private/server_udp_api_p.h"
#include "server_certificate.h"
#include "private/storage_log_p.h"
#include "transaction_block.h"
#include "transaction_block.h"
#include "transaction_context.h"
#include "chunk_manager.h"
#include "transaction_log.h"
#include "db_model.h"
#include "certificate_dbo.h"
#include "certificate_private_key_dbo.h"
#include "p2p_network_client.h"
#include "run_configuration_dbo.h"

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
  
  registrator.add_service<ichunk_manager>(this->impl_->chunk_manager_.get());
  
  registrator.add_service<iserver_database>(this->impl_->server_database_.get());
  
  registrator.add_service<ilocal_cache>(this->impl_->local_cache_.get());

  registrator.add_service<node_manager>(this->impl_->node_manager_.get());

  registrator.add_service<cert_manager>(this->impl_->cert_manager_.get());

  registrator.add_service<user_manager>(this->impl_->user_manager_.get());

  registrator.add_service<db_model>(this->impl_->db_model_.get());

  registrator.add_service<ip2p_network_client>(this->impl_->network_client_.get());
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
    auto usr_manager = sp.get<user_manager>();
    auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
    auto block_data = usr_manager->reset(sp, t, root_user_name, root_password, private_key);
    auto block_id = chunk_manager::pack_block(t, block_data);

	  transaction_log::apply(sp, t, chunk_manager::get_block(t, block_id));

    usr_manager->lock_to_device(sp, t, root_user_name, root_password, 8050);

  });
}

vds::async_task<> vds::server::init_server(const vds::service_provider &sp, int port, const std::string &user_login,
                                           const std::string &user_password) {
  return this->impl_->init_server(sp, port, user_login, user_password);

}

vds::async_task<> vds::server::start_network(const vds::service_provider &sp) {
  return this->impl_->start_network(sp);
}

void vds::transaction_log::apply(
    const service_provider &sp,
    database_transaction &t,
    const const_data_buffer &chunk) {
  auto scope = sp.create_scope("Apply record");

  auto data = transaction_block::unpack_block(
      scope,
      chunk,
    [&t](const guid & cert_id) -> certificate{
	  certificate_dbo t1;
	  auto st = t.get_reader(t1.select(t1.cert).where(t1.id == cert_id));
	  if (st.execute()) {
		  return certificate::parse_der(t1.cert.get(st));
	  }
	  else {
		  return certificate();
	  }
    },
    [&t](const guid & cert_id) -> asymmetric_private_key{
		certificate_private_key_dbo t1;
		auto st = t.get_reader(t1.select(t1.body).where(t1.id == cert_id));
		if (st.execute()) {
			return asymmetric_private_key::parse_der(t1.body.get(st), std::string());
		}
		else {
			return asymmetric_private_key();
		}
	});

  binary_deserializer s(data);

  while(0 < s.size()){
    uint8_t category_id;
    uint8_t message_id;
    s >> category_id >> message_id;
    switch (category_id){
      case transaction_log::user_manager_category_id:
      {
        scope.get<user_manager>()->apply_transaction_record(scope, t, message_id, s);
        break;
      }
      default:
        throw std::runtime_error("Invalid record category");
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////

vds::_server::_server(server * owner)
: owner_(owner),
  node_manager_(new _node_manager()),
  server_http_api_(new _server_http_api()),
  storage_log_(new _storage_log()),
  chunk_manager_(new _chunk_manager()),
  server_database_(new _server_database()),
  local_cache_(new _local_cache()),
	user_manager_(new user_manager()),
	db_model_(new db_model()),
  network_client_(new p2p_network_client())
{
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
	this->db_model_->stop(sp);
}

vds::async_task<> vds::_server::init_server(
    const vds::service_provider &sp,
    int port,
    const std::string &user_name,
    const std::string &user_password) {
  this->network_services_.push_back(p2p_network_service());
  return this->network_services_.rbegin()->start(sp, port, user_name, user_password);
}

struct run_data
{
  int port;
  vds::certificate cert;
  vds::asymmetric_private_key key;
};

vds::async_task<> vds::_server::start_network(const vds::service_provider &sp) {

  imt_service::enable_async(sp);
  auto run_conf = std::make_shared<std::list<run_data>>();
  return sp.get<db_model>()->async_transaction(sp, [run_conf](database_transaction & t){
    run_configuration_dbo t1;
    certificate_dbo t2;
    certificate_private_key_dbo t3;
    auto st = t.get_reader(
        t1.select(t1.port, t2.cert, t3.body)
            .inner_join(t2, t2.id == t1.cert_id)
            .inner_join(t3, t3.id == t1.cert_id));
    while(st.execute()){
      run_conf->push_back( run_data {
          .port = t1.port.get(st),
          .cert = certificate::parse_der(t2.cert.get(st)),
          .key = asymmetric_private_key::parse_der(t3.body.get(st), std::string())
      });
    }

    return true;
  }).then([sp, run_conf, this](){
    if(run_conf->empty()){
      throw std::runtime_error("There is no active network configuration");
    }

    auto result = async_task<>::empty();
    for(auto & conf : *run_conf) {
      this->network_services_.push_back(p2p_network_service());
      result = result.then([this, sp, conf]() {
        return this->network_services_.rbegin()->start(sp, conf.port, conf.cert, conf.key);
      });
    }
    return result;
  });
}
