/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "server_p.h"
#include "node_manager.h"
#include "user_manager.h"
#include "cert_manager.h"
#include "server_http_api.h"
#include "server_http_api_p.h"
#include "server_connection.h"
#include "server_udp_api.h"
#include "node_manager.h"
#include "node_manager_p.h"
#include "storage_log_p.h"
#include "chunk_manager_p.h"
#include "server_database_p.h"
#include "local_cache_p.h"
#include "node_manager_p.h"
#include "cert_manager_p.h"
#include "server_connection_p.h"
#include "server_udp_api_p.h"
#include "server_certificate.h"
#include "storage_log_p.h"

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

void vds::server::set_port(size_t port)
{
  this->impl_->set_port(port);
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
    .wait(
      [](const service_provider& sp) {sp.get<logger>()->trace(sp, "Server closed"); },
      [](const service_provider& sp, const std::shared_ptr<std::exception> & ex) {
        sp.get<logger>()->trace(sp, "Server error %s", ex->what());
        sp.unhandled_exception(ex);
      },
      scope);
}

void vds::_server::stop(const service_provider& sp)
{
  this->server_http_api_->stop(sp);
  this->chunk_manager_->stop(sp);
  this->local_cache_->stop(sp);
  this->storage_log_->stop(sp);
  this->server_database_->stop(sp);
}

void vds::_server::set_port(size_t port)
{
  this->port_ = port;
}
