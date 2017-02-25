/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "vsr_protocol.h"
#include "node_manager.h"
#include "user_manager.h"
#include "cert_manager.h"
#include "server_http_api.h"
#include "_server_http_api.h"

vds::server::server()
{
}

vds::server::~server()
{
}

void vds::server::register_services(service_registrator& registrator)
{
  registrator.add_factory<iserver>([this](const service_provider &, bool &)->iserver{
    return iserver(this);
  });
  
  registrator.add_factory<vsr_protocol::iserver>([this](const service_provider &, bool &)->vsr_protocol::iserver{
    return vsr_protocol::iserver(this->vsr_server_protocol_.get());
  });
}

void vds::server::start(const service_provider& sp)
{
  this->storage_log_.reset(new storage_log(sp));
  this->storage_log_->start();
  
  this->vsr_server_protocol_.reset(new vsr_protocol::server(sp));
  this->vsr_server_protocol_->start();

  this->node_manager_.reset(new node_manager(sp));
  this->server_http_api_.reset(new server_http_api(sp));
}

void vds::server::stop(const service_provider& sp)
{

}

vds::iserver::iserver(vds::server* owner)
: owner_(owner)
{

}

void vds::iserver::start_http_server(const std::string & address, int port)
{
  this->owner_->server_http_api_->start(address, port);
}
