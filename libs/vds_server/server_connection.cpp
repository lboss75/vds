/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_connection.h"
#include "server_connection_p.h"
#include "server.h"
#include "node_manager.h"
#include "server_udp_api.h"

vds::server_connection::server_connection(
  const service_provider & sp,
  server_udp_api * udp_api)
  : impl_(new _server_connection(sp, udp_api, this))
{

}

vds::server_connection::~server_connection()
{
  delete this->impl_;
}

void vds::server_connection::start()
{
  this->impl_->start();
}

void vds::server_connection::stop()
{
  this->impl_->stop();
}


///////////////////////////////////////////////////////////////////////////////////
vds::_server_connection::_server_connection(
  const service_provider & sp,
  server_udp_api * udp_api,
  server_connection * owner)
: sp_(sp), 
  log_(sp, "Server Connection"),
  udp_api_(udp_api),
  owner_(owner)
{
}

vds::_server_connection::~_server_connection()
{
}

void vds::_server_connection::start()
{
  std::map<std::string, std::string> endpoints;
  this->sp_.get<iserver>().get_node_manager().get_endpoints(endpoints);
  
  for (auto & p : endpoints) {
    this->log_.info("Connecting to %s", p.first.c_str());
    
    url_parser::parse_addresses(p.second,
      [this](const std::string & protocol, const std::string & address) -> bool {
        if("udp" == protocol){
          this->udp_api_->open_udp_session(address);
        }
        else if ("https" == protocol) {
          this->open_https_session(address);
        }
      
        return true;
    });
  }
}

void vds::_server_connection::stop()
{
}

void vds::_server_connection::get_delivery_metrics(
  std::map<std::string, size_t> & metrics)
{
}
  
void vds::_server_connection::send(
  const std::string & from_address,
  std::list<std::string> & to_address,
  const std::string &  body)
{
}

void vds::_server_connection::init_connection(const std::string & address, uint16_t port)
{

}


void vds::_server_connection::open_udp_session(const std::string & address)
{
}

void vds::_server_connection::open_https_session(const std::string & address)
{
  auto network = url_parser::parse_network_address(address);
  assert("https" == network.protocol);
  
}

