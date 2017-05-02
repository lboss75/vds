/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "node_manager.h"
#include "node_manager_p.h"
#include "storage_log.h"

vds::async_task<> vds::node_manager::register_server(
  const service_provider & scope,
  const std::string & node_certificate)
{
  return static_cast<_node_manager *>(this)->register_server(scope, node_certificate);
}

void vds::node_manager::add_endpoint(
  const service_provider & sp,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  static_cast<_node_manager *>(this)->add_endpoint(sp, endpoint_id, addresses);
}

void vds::node_manager::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string> & addresses)
{
  static_cast<_node_manager *>(this)->get_endpoints(sp, addresses);
}

///////////////////////////////////////////////////////////////////////////////
vds::_node_manager::_node_manager()
{
}

vds::async_task<> vds::_node_manager::register_server(
  const service_provider & sp,
  const std::string & node_certificate)
{
  return sp.get<istorage_log>().register_server(sp, node_certificate);
}

void vds::_node_manager::add_endpoint(
  const service_provider & sp,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  sp.get<istorage_log>().add_endpoint(sp, endpoint_id, addresses);
}

void vds::_node_manager::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string> & addresses)
{
  sp.get<istorage_log>().get_endpoints(sp, addresses);
}
