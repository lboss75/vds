/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "route_manager.h"
#include "route_manager_p.h"

vds::route_manager::route_manager()
  : impl_(new _route_manager())
{
}

vds::route_manager::~route_manager()
{
  delete this->impl_;
}

void vds::route_manager::add_route(
  const service_provider& sp,
  const guid& source_server_id,
  const guid& target_server_id,
  const std::string& address)
{

}

void vds::route_manager::get_routes(
  const service_provider& sp,
  const guid& target_server_id,
  const std::function<bool(size_t metric, std::list<network_route> & routes)> & callback)
{

}

vds::route_message::route_message(
  const guid & target_server_id,
  const std::string & address,
  const std::chrono::steady_clock & last_access)
{

}
//////////////////////////////////
vds::_route_manager::_route_manager()
{
}

vds::_route_manager::~_route_manager()
{
}

void vds::_route_manager::on_session_started(
  const service_provider& sp,
  const guid & source_server_id,
  const guid & target_server_id,
  const std::string & address)
{
}

