/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "object_transfer_protocol.h"
#include "object_transfer_protocol_p.h"
#include "connection_manager.h"

vds::object_transfer_protocol::object_transfer_protocol()
: impl_(new _object_transfer_protocol())
{

}

vds::object_transfer_protocol::~object_transfer_protocol()
{
  delete this->impl_;
}

void vds::object_transfer_protocol::ask_object(
  const vds::service_provider& sp,
  const vds::guid & source_server_id,
  uint64_t object_index)
{
  object_request message(source_server_id, object_index);
  sp.get<iconnection_manager>()->broadcast(sp, message);
}

//////////////////////////////////////////////////////
vds::_object_transfer_protocol::_object_transfer_protocol()
{
}

vds::_object_transfer_protocol::~_object_transfer_protocol()
{
}

void vds::_object_transfer_protocol::on_object_request(
  const service_provider & sp,
  const object_request & message)
{
  auto connection_manager = sp.get<iconnection_manager>();
  
  auto replicas = sp.get<ilocal_cache>()->get_local_replicas(sp, message.server_id(), message.index());
  if(!replicas.empty()){
    
  }
  
}


