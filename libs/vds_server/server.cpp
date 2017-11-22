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
}

void vds::server::start(const service_provider& sp)
{
  this->impl_->start(sp);
}

void vds::server::stop(const service_provider& sp)
{
  this->impl_->stop(sp);
}

void vds::server::set_port(int port)
{
  this->impl_->set_port(port);
}

vds::async_task<> vds::server::reset(
    const vds::service_provider &sp,
    const std::string &root_user_name,
    const std::string &root_password) {

  auto usr_manager = sp.get<user_manager>();
  auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  auto block_data = usr_manager->reset(sp, root_user_name, root_password, private_key);
  return sp.get<db_model>()->async_transaction(sp, [this, sp, block_data](database_transaction & t){
    auto block_id = chunk_manager::pack_block(t, block_data);

	transaction_log::apply(sp, t, chunk_manager::get_block(t, block_id));
  });
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
        sp.get<user_manager>()->apply_transaction_record(sp, t, message_id, s);
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
  local_cache_(new _local_cache())
{
}

vds::_server::~_server()
{
}

void vds::_server::start(const service_provider& sp)
{
  this->certificate_.load(filename(foldername(persistence::current_user(sp), ".vds"), "server.crt"));
  this->private_key_.load(filename(foldername(persistence::current_user(sp), ".vds"), "server.pkey"));

  this->server_database_->start(sp);
  this->storage_log_->start(
    sp,
    server_certificate::server_id(this->certificate_),
    this->certificate_,
    this->private_key_);
  this->local_cache_->start(sp);
  this->chunk_manager_->start(sp);

  auto scope = sp.create_scope("Server HTTP API");
  imt_service::enable_async(scope);
  this->server_http_api_->start(sp, "127.0.0.1", this->port_, this->certificate_, this->private_key_)
    .execute(
      [sp](const std::shared_ptr<std::exception> & ex) {
        if(!ex){
          sp.get<logger>()->trace("HTTP API", sp, "Server closed");
        } else {
          sp.get<logger>()->trace("HTTP API", sp, "Server error %s", ex->what());
          sp.unhandled_exception(ex);
        }
      });
}

void vds::_server::stop(const service_provider& sp)
{
  this->server_http_api_->stop(sp);
  this->chunk_manager_->stop(sp);
  this->local_cache_->stop(sp);
  this->storage_log_->stop(sp);
  this->server_database_->stop(sp);
}

void vds::_server::set_port(int port)
{
  this->port_ = port;
}
