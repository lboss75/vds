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
  std::unique_lock<std::mutex> lock(this->states_mutex_);

  auto p = this->states_.find(node_certificate);
  if (this->states_.end() != p) {
    switch(p->second){
    case _node_manager::pending:
    {
      storage_cursor<node> node_cursor(scope.get<istorage>());
      while(node_cursor.read()){
        this->log_(ll_trace, "have node %s", node_cursor.current().id().c_str());
        if(node_cursor.current().certificate() == node_certificate){
          p->second = _node_manager::complete;
          return true;
        }
      }
      
      return false;
    }
    case _node_manager::error:
      error = "failed";
      return true;
    case _node_manager::complete:
      return true;
    default:
      throw new std::logic_error("Invalid state of the node_manager");
    }
  }

  this->states_.set(node_certificate, _node_manager::pending);

  scope.get<istorage>().get_storage_log().register_server(node_certificate);
  return false;
}

