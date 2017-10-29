/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "object_transfer_protocol.h"
#include "private/object_transfer_protocol_p.h"
#include "connection_manager.h"
#include "private/server_database_p.h"
#include "storage_log.h"
#include "private/chunk_manager_p.h"

vds::object_transfer_protocol::object_transfer_protocol()
: impl_(new _object_transfer_protocol())
{

}

vds::object_transfer_protocol::~object_transfer_protocol()
{
  delete this->impl_;
}

/////////////////////////////////////////////
const char vds::object_request::message_type[] = "object request";

vds::object_request::object_request(const const_data_buffer & binary_form)
{
  binary_deserializer s(binary_form);
  s >> this->server_id_ >> this->index_ >> this->target_storage_id_ >> this->replicas_;
}

void vds::object_request::serialize(binary_serializer & b) const
{
  b << this->server_id_ << this->index_ << this->target_storage_id_ << this->replicas_;
}

std::shared_ptr<vds::json_value> vds::object_request::serialize() const
{
  auto result = std::make_shared<json_object>();
  result->add_property("$t", message_type);
  result->add_property("s", this->server_id_);
  result->add_property("i", this->index_);
  result->add_property("t", this->target_storage_id_);
  result->add_property("r", this->replicas_);
  return result;
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
  database_transaction & tr,
  const connection_session & session,
  const object_request & message)
{
  sp.get<logger>()->debug("otp", sp, "route: request object %s:%d for %s", message.server_id().str().c_str(), message.index(), message.target_storage_id().str().c_str());

  auto current_server_id = sp.get<istorage_log>()->current_server_id();
  
  auto chunk_manager = sp.get<ichunk_manager>();


  auto connection_manager = sp.get<iconnection_manager>();
  std::list<ichunk_manager::replica_type> local_replicas;
  (*chunk_manager)->get_replicas(
    sp,
    tr,
    message.server_id(),
    message.index(),
    current_server_id,
    local_replicas);

  for(auto replica : message.replicas()){
    local_replicas.remove(replica);
  }
  
  if (local_replicas.empty()) {
    sp.get<logger>()->debug("otp", sp, "No local chunks");
    return;
  }
  
  for(auto replica : local_replicas){
    auto data = (*chunk_manager)->get_replica_data(
      sp,
      tr,
      message.server_id(),
      message.index(),
      replica);
    
    if(!data){
      continue;
    }
    
    sp.get<logger>()->debug("otp", sp, "Route: send replica %s:%d.%d to %s",
      message.server_id().str().c_str(),
      message.index(),
      replica,
      message.target_storage_id().str().c_str());
    connection_manager->send_to(
      sp,
      message.target_storage_id(), 
      object_offer_replicas(
        message.server_id(),
        message.index(),
        replica,
        data));
  }
}

void vds::_object_transfer_protocol::object_offer(
  const service_provider & sp,
  database_transaction & tr,
  const connection_session & session,
  const object_offer_replicas & message)
{
  sp.get<logger>()->debug("otp", sp, "Route: got object %s:%d.%d", 
    message.server_id().str().c_str(),
    message.index(),
    message.replica());

  auto storage_log = sp.get<istorage_log>();
  auto current_server_id = storage_log->current_server_id();
  auto chunk_manager = sp.get<ichunk_manager>();
  
  const_data_buffer replica_hash;
  bool replica_stored;
  if ((*chunk_manager)->get_replica_info(
    sp,
    tr,
    message.server_id(),
    message.index(),
    message.replica(),
    replica_hash,
    replica_stored) && !replica_stored){

    //TODO: Validate data hash
    sp.get<istorage_log>()->add_to_local_log(
      sp,
      tr,
      current_server_id,
      current_server_id,
      storage_log->server_private_key(),
      principal_log_store_replica(
        message.server_id(),
        message.index(),
        message.replica()).serialize(true));
  }
}
////////////////////////////////////////////////////////
vds::object_offer_replicas::object_offer_replicas(const const_data_buffer & binary_form)
{
  binary_deserializer b(binary_form);
  b >> this->server_id_ >> this->index_ >> this->replica_ >> this->data_;
}

void vds::object_offer_replicas::serialize(
  binary_serializer & b) const
{
  b << this->server_id_ << this->index_ << this->replica_ << this->data_;
}
////////////////////////////////////////////////////////
