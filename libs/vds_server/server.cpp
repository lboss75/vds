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
#include "server_connection.h"
#include "server_udp_api.h"

vds::server::server()
: port_(8050)
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
  
  registrator.add_factory<istorage>([this](const service_provider &, bool &)->istorage{
    return istorage(this->storage_log_.get());
  });
}

void vds::server::start(const service_provider& sp)
{
  this->certificate_.load(filename(foldername(persistence::current_user(sp), ".vds"), "server.crt"));
  this->private_key_.load(filename(foldername(persistence::current_user(sp), ".vds"), "server.pkey"));
  
  this->storage_log_.reset(new storage_log(
    sp,
    server_certificate::server_id(this->certificate_),
    this->certificate_,
    this->private_key_));
  this->storage_log_->start();

  this->connection_manager_.reset(new connection_manager(sp, this->certificate_.subject()));

  this->consensus_server_protocol_.reset(new consensus_protocol::server(sp, this->certificate_, this->private_key_, *this->connection_manager_));
  this->consensus_server_protocol_->start();

  this->node_manager_.reset(new node_manager(sp));
  this->server_http_api_.reset(new server_http_api(sp));
  this->server_http_api_->start("127.0.0.1", this->port_, this->certificate_, this->private_key_);
  
  this->server_udp_api_.reset(new server_udp_api(sp, this->certificate_, this->private_key_));
  this->server_udp_api_->start("127.0.0.1", this->port_);
  
  this->server_connection_.reset(new server_connection(sp, this->server_udp_api_.get()));
  this->server_connection_->start();
  
  this->peer_network_.reset(new peer_network(sp));
  //this->peer_network_->start();
  

  //this->client_logic_.reset(new client_logic(sp, &this->certificate_, &this->private_key_, sp.get<istorage>().get_storage_log().get_endpoints()));
  //this->client_logic_->start();
}

void vds::server::stop(const service_provider& sp)
{

}

void vds::server::set_port(size_t port)
{
  this->port_ = port;
}

vds::iserver::iserver(vds::server* owner)
: owner_(owner)
{

}
