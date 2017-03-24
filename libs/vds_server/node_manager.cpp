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

bool vds::node_manager::register_server(const service_provider & scope, const std::string & node_certificate, std::string & error)
{
  return this->impl_->register_server(scope, node_certificate, error);
}

///////////////////////////////////////////////////////////////////////////////
vds::_node_manager::_node_manager(const service_provider & sp)
  : sp_(sp),
  log_(sp, "Node manager")
{
}

bool vds::_node_manager::register_server(const service_provider & scope, const std::string & node_certificate, std::string & error)
{
  scope.get<istorage>().get_storage_log().register_server(node_certificate);
  return true;
}

