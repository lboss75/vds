/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "object_transfer_protocol.h"
#include "object_transfer_protocol_p.h"
#include "connection_manager.h"
#include "server_database_p.h"
#include "storage_log.h"
#include "chunk_manager_p.h"

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
  auto current_server_id = sp.get<istorage_log>()->current_server_id();

  auto chunk_manager = sp.get<ichunk_manager>();
  std::list<ichunk_manager::replica_type> local_replicas;
  (*chunk_manager)->get_replicas(
    sp,
    source_server_id,
    chunk_index,
    current_server_id,
    local_replicas);

  std::list<ichunk_manager::chunk_store> chunk_store_map;
  (*chunk_manager)->get_chunk_store(
    sp,
    source_server_id,
    chunk_index,
    chunk_store_map);

  object_request message(source_server_id, chunk_index);
  auto connection_manager = sp.get<iconnection_manager>();
  for (auto & item : chunk_store_map) {
    if (current_server_id != item.storage_id
      && local_replicas.end() == std::find(local_replicas.begin(), local_replicas.end(), item.replica)) {

      connection_manager->send_to(sp, item.storage_id, message);
    }
  }
}

//////////////////////////////////////////////////////
vds::_object_transfer_protocol::_object_transfer_protocol()
{
}

vds::_object_transfer_protocol::~_object_transfer_protocol()
{
}

/*
void vds::_object_transfer_protocol::on_object_request(
  const service_provider & sp,
  const guid & from_server_id,
  const object_request & message)
{
  auto current_server_id = sp.get<istorage_log>()->current_server_id();

  std::list<ichunk_manager::replica_type> local_replicas;
  (*sp.get<ichunk_manager>())->get_replicas(
    sp,
    message.server_id(),
    message.index(),
    current_server_id,
    local_replicas);

  auto connection_manager = sp.get<iconnection_manager>();
  if (!local_replicas.empty()) {
    object_offer_replicas message(
      source_server_id,
      chunk_index,
      current_server_id,
      local_replicas);
    
    connection_manager->send_to(message.hops(), message);
  }

  std::list<iserver_database::chunk_store> chunk_store_map;
  sp.get<iserver_database>()->get_chunk_store(
    sp,
    source_server_id,
    chunk_index,
    chunk_store_map);

  object_request message(source_server_id, chunk_index);
  for (auto & item : chunk_store_map) {
    if (current_server_id != item.storage_id
      && local_replicas.end() == std::find(local_replicas.begin(), local_replicas.end(), item.replica)) {

      connection_manager->send_to(sp, item.storage_id, message);
    }
  }
}
*/

