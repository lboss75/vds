/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "object_transfer_protocol.h"
#include "object_transfer_protocol_p.h"
#include "connection_manager.h"
#include "server_database.h"

vds::object_transfer_protocol::object_transfer_protocol()
: impl_(new _object_transfer_protocol())
{

}

vds::object_transfer_protocol::~object_transfer_protocol()
{
  delete this->impl_;
}

void vds::object_transfer_protocol::chunk_require(
  const vds::service_provider& sp,
  const vds::guid & source_server_id,
  uint64_t chunk_index)
{
  std::list<iserver_database::chunk_store> chunk_store_map;
  sp.get<iserver_database>()->get_chunk_store(
    sp,
    source_server_id,
    chunk_index,
    chunk_store_map);
  
  object_request message(source_server_id, chunk_index);
  auto connection_manager = sp.get<iconnection_manager>();
  for(auto & item : chunk_store_map){
    connection_manager->send_to(item.storage_id, message);
  }
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


