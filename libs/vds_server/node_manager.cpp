/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "node_manager.h"
#include "node_manager_p.h"
#include "server.h"

vds::node_manager::node_manager(const service_provider & sp)
  : impl_(new _node_manager(sp))
{
}

vds::node_manager::~node_manager()
{
}

vds::async_task<> vds::node_manager::register_server(const service_provider & scope, const std::string & node_certificate)
{
  return this->impl_->register_server(scope, node_certificate);
}

void vds::node_manager::add_endpoint(
  const std::string & endpoint_id,
  const std::string & addresses)
{
  this->impl_->add_endpoint(endpoint_id, addresses);
}

void vds::node_manager::get_endpoints(std::map<std::string, std::string> & addresses)
{
  this->impl_->get_endpoints(addresses);
}

///////////////////////////////////////////////////////////////////////////////
vds::_node_manager::_node_manager(const service_provider & sp)
  : sp_(sp),
  log_(sp, "Node manager")
{
}

vds::async_task<> vds::_node_manager::register_server(const service_provider & scope, const std::string & node_certificate)
{
  return scope.get<istorage_log>().register_server(node_certificate);
}

void vds::_node_manager::add_endpoint(
  const std::string & endpoint_id,
  const std::string & addresses)
{
  this->sp_.get<istorage_log>().add_endpoint(endpoint_id, addresses);
}

void vds::_node_manager::get_endpoints(std::map<std::string, std::string> & addresses)
{
  this->sp_.get<istorage_log>().get_endpoints(addresses);
}
