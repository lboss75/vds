/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "consensus_protocol.h"
#include "node_manager.h"
#include "user_manager.h"
#include "cert_manager.h"
#include "server_http_api.h"
#include "server_http_api_p.h"

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
}

void vds::server::start(const service_provider& sp)
{
  this->certificate_.load(filename(foldername(persistence::current_user(sp), ".vds"), "server.crt"));
  this->private_key_.load(filename(foldername(persistence::current_user(sp), ".vds"), "server.pkey"));

  this->consensus_server_protocol_.reset(new consensus_protocol::server(sp, this->certificate_, this->private_key_));
  this->consensus_server_protocol_->start();

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
  this->owner_->server_http_api_->start(address, port, this->owner_->certificate_, this->owner_->private_key_);
}
