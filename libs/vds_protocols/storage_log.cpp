/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "storage_log.h"
#include "storage_log_p.h"
#include "process_log_line.h"
#include "log_records.h"
#include "principal_record.h"
#include "node.h"
#include "endpoint.h"
#include "certificate_authority.h"
#include "certificate_authority_p.h"
#include "chunk_storage.h"
#include "server_log_sync.h"
#include "server_log_sync_p.h"
#include "server_certificate.h"
#include "chunk_manager_p.h"
#include "principal_manager_p.h"
#include "server_database_p.h"

const vds::guid & vds::istorage_log::current_server_id() const
{
  return static_cast<const _storage_log *>(this)->current_server_id();
}

const vds::certificate & vds::istorage_log::server_certificate() const
{
  return static_cast<const _storage_log *>(this)->server_certificate();
}

const vds::asymmetric_private_key & vds::istorage_log::server_private_key() const
{
  return static_cast<const _storage_log *>(this)->server_private_key();
}

size_t vds::istorage_log::minimal_consensus() const
{
  return static_cast<const _storage_log *>(this)->minimal_consensus();
}

void vds::istorage_log::add_to_local_log(
  const service_provider & sp,
  const guid & principal_id,
  const vds::asymmetric_private_key & principal_private_key,
  const std::shared_ptr<json_value> & record,
  bool apply_record /*= true*/)
{
  static_cast<_storage_log *>(this)->add_to_local_log(sp, principal_id, principal_private_key, record, apply_record);
}

vds::async_task<> vds::istorage_log::register_server(
  const service_provider & sp,
  const guid & id,
  const guid & parent_id,
  const std::string & server_certificate,
  const std::string & server_private_key,
  const const_data_buffer & password_hash)
{
  return static_cast<_storage_log *>(this)->register_server(
    sp,
    id,
    parent_id,
    server_certificate,
    server_private_key,
    password_hash);
}

std::unique_ptr<vds::const_data_buffer> vds::istorage_log::get_object(
  const service_provider & sp,
  const vds::full_storage_object_id& object_id)
{
  return static_cast<_storage_log *>(this)->get_object(sp, object_id);
}

void vds::istorage_log::add_endpoint(
  const service_provider & sp,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  static_cast<_storage_log *>(this)->add_endpoint(sp, endpoint_id, addresses);
}

void vds::istorage_log::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string> & addresses)
{
  static_cast<_storage_log *>(this)->get_endpoints(sp, addresses);
}

void vds::istorage_log::reset(
  const service_provider & sp,
  const guid & principal_id,
  const vds::certificate & root_certificate,
  const asymmetric_private_key & private_key,
  const std::string & root_password,
  const std::string & addresses)
{
  static_cast<_storage_log *>(this)->reset(
    sp,
    principal_id,
    root_certificate,
    private_key,
    root_password,
    addresses);
}

void vds::istorage_log::apply_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  static_cast<_storage_log *>(this)->apply_record(
    sp,
    record,
    signature);
}

vds::principal_log_record::record_id vds::istorage_log::get_last_applied_record(const service_provider & sp)
{
  return static_cast<_storage_log *>(this)->get_last_applied_record(sp);
}

///////////////////////////////////////////////////////////////////////////////
vds::_storage_log::_storage_log()
{
}

vds::_storage_log::~_storage_log()
{
}

void vds::_storage_log::reset(
  const service_provider & sp,
  const guid & principal_id,
  const certificate & root_certificate,
  const asymmetric_private_key & private_key,
  const std::string & password,
  const std::string & addresses
)
{
  this->vds_folder_ = foldername(persistence::current_user(sp), ".vds");
  this->vds_folder_.create();
  
  server_database_scope scope(sp);
  
  hash ph(hash::sha256());
  ph.update(password.c_str(), password.length());
  ph.final();
  
  this->add_to_local_log(
    scope,
    principal_id,
    private_key,
    server_log_root_certificate(
      principal_id,
      root_certificate.str(),
      private_key.str(password),
      ph.signature()).serialize(),
      true);
  this->add_to_local_log(scope, principal_id, private_key, server_log_new_server(
    this->current_server_id_,
    principal_id,
    this->server_certificate_.str(),
    this->current_server_key_.str(password),
    ph.signature()).serialize(),
    true);
  this->add_to_local_log(
    scope,
    principal_id,
    private_key,
    server_log_new_endpoint(this->current_server_id_, addresses).serialize(),
    true);
  
  scope.commit();
}


void vds::_storage_log::start(
  const service_provider & sp,
  const guid & current_server_id,
  const certificate & server_certificate,
  const asymmetric_private_key & server_private_key)
{
  this->server_certificate_ = server_certificate;
  this->current_server_key_ = server_private_key;
  this->current_server_id_ = current_server_id;
  this->vds_folder_ = foldername(persistence::current_user(sp), ".vds");
  this->process_timer_.start(sp, std::chrono::seconds(5), [this, sp](){
    return this->process_timer_jobs(sp);
  });
}

void vds::_storage_log::stop(const service_provider & sp)
{
  this->process_timer_.stop(sp);
}

vds::async_task<> vds::_storage_log::register_server(
  const service_provider & sp,
  const guid & id,
  const guid & parent_id,
  const std::string & server_certificate,
  const std::string & server_private_key,
  const const_data_buffer & password_hash)
{
  return create_async_task(
    [this, id, parent_id, server_certificate, server_private_key, password_hash](
      const std::function<void(const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp) {
    this->add_to_local_log(
      sp,
      this->current_server_id(),
      this->server_private_key(),
      server_log_new_server(
        id,
        parent_id,
        server_certificate,
        server_private_key,
        password_hash).serialize(),
      true);
    done(sp);
  });
}

void vds::_storage_log::add_to_local_log(
  const service_provider & sp,
  const guid & principal_id,
  const vds::asymmetric_private_key & principal_private_key,
  const std::shared_ptr<json_value> & message,
  bool apply_record)
{
  std::lock_guard<std::mutex> lock(this->record_state_mutex_);

  const_data_buffer signature;
  auto result = this->principal_manager_->add_local_record(
      sp,
      guid::new_guid(),
      principal_id,
      message,
      principal_private_key,
      signature);
    
  if(apply_record){
    this->apply_record(sp, result, signature);
  }
  else {
    this->last_applied_record_ = result.id();
    this->principal_manager_->processed_record(sp, result.id());
  }

  auto sl = sp.get<_server_log_sync>(false);
  if (nullptr != sl) {
    sl->on_new_local_record(sp, result, signature);
  }
}

void vds::_storage_log::apply_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  sp.get<logger>()->debug(sp, "Apply record %s", record.id().str().c_str());
  auto state = this->principal_manager_->principal_log_get_state(sp, record.id());
  if(_principal_manager::principal_log_state::front != state){
    throw std::runtime_error("Invalid server state");
  }
 
  auto obj = std::dynamic_pointer_cast<json_object>(record.message());
  if(!obj){
    sp.get<logger>()->info(sp, "Wrong messsage in the record %s", record.id().str().c_str());
    this->principal_manager_->delete_record(sp, record.id());
    return;
  }
  
  try {
    this->validate_signature(sp, record, signature);

    std::string message_type;
    if (!obj->get_property("$t", message_type)) {
      sp.get<logger>()->info(sp, "Missing messsage type in the record %s", record.id().str().c_str());
      this->principal_manager_->delete_record(sp, record.id());
      return;
    }
    
    if (principal_log_new_object::message_type == message_type) {
      principal_log_new_object msg(obj);
      sp.get<iserver_database>()->add_object(sp, msg);
    }
    else if (principal_log_new_object_map::message_type == message_type) {
      principal_log_new_object_map msg(obj);
      
      size_t offset = 0;
      for(auto & chunk : msg.full_chunks()){
        (*sp.get<ichunk_manager>())->add_full_chunk(
          sp,
          msg.object_id(),
          offset,
          msg.chunk_size(),
          chunk.chunk_hash(),
          msg.server_id(),
          chunk.chunk_index());
        
        size_t replica = 0;
        for(auto & replica_hash : chunk.replica_hashes()){
          (*sp.get<ichunk_manager>())->add_chunk_replica(
            sp,
            msg.server_id(),
            chunk.chunk_index(),
            replica++,
            msg.replica_length(),
            replica_hash);
        }
        
        offset += msg.chunk_size();
      }
      
      for(auto & tail_chunk : msg.tail_chunk_items()){
        (*sp.get<ichunk_manager>())->add_object_chunk_map(
          sp,
          tail_chunk.server_id(),
          tail_chunk.chunk_index(),
          tail_chunk.object_id(),
          tail_chunk.object_offset(),
          tail_chunk.chunk_offset(),
          tail_chunk.length(),
          tail_chunk.hash());
      }
    }
    else if(server_log_root_certificate::message_type == message_type){
      server_log_root_certificate msg(obj);
      
      this->principal_manager_->add_user_principal(
        sp,
        "root",
        vds::principal_record(
          msg.id(),
          msg.id(),
          msg.user_cert(),
          msg.user_private_key(),
          msg.password_hash()));
    }
    else if(server_log_new_server::message_type == message_type){
      server_log_new_server msg(obj);

      this->principal_manager_->add_principal(
        sp,
        vds::principal_record(
          msg.parent_id(),
          msg.id(),
          msg.server_cert(),
          msg.server_private_key(),
          msg.password_hash()));

    }
    else if(server_log_new_endpoint::message_type == message_type){
      server_log_new_endpoint msg(obj);
      
      sp.get<iserver_database>()->add_endpoint(
        sp,
        msg.server_id().str(),
        msg.addresses());
    }
    else {
      sp.get<logger>()->info(sp, "Unexpected messsage type '%s' in the record %s",
        message_type.c_str(),
        record.id().str().c_str());

      this->principal_manager_->delete_record(sp, record.id());
      return;
    }

    this->last_applied_record_ = record.id();
  }
  catch (const std::exception & ex) {
    sp.get<logger>()->info(sp, "%s", ex.what());
    this->principal_manager_->delete_record(sp, record.id());
    return;
  }
  catch(...){
    sp.get<logger>()->info(sp, "Unhandled error");
    this->principal_manager_->delete_record(sp, record.id());
    return;
  }
  
  this->principal_manager_->processed_record(sp, record.id());
}

std::unique_ptr<vds::const_data_buffer> vds::_storage_log::get_object(
  const service_provider & sp,
  const vds::full_storage_object_id& object_id)
{
  return sp.get<ilocal_cache>()->get_object(sp, object_id);
}

void vds::_storage_log::add_endpoint(
  const service_provider & sp,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  sp.get<iserver_database>()->add_endpoint(sp, endpoint_id, addresses);
}

void vds::_storage_log::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string> & addresses)
{
  sp.get<iserver_database>()->get_endpoints(sp, addresses);
}


  /*
  auto signature = asymmetric_sign::signature(hash::sha256(), this->current_server_key_, s.data());
  this->db_.add_object(this->current_server_id_, index, signature);
  return vds::storage_object_id(index, signature);
  */


bool vds::_storage_log::process_timer_jobs(const service_provider & sp)
{
  std::lock_guard<std::mutex> lock(this->record_state_mutex_);

  server_database_scope scope(sp);
  
  principal_log_record record;
  const_data_buffer signature;
  while(this->principal_manager_->get_front_record(scope, record, signature)){
    this->apply_record(scope, record, signature);
  }

  scope.commit();
  return true;
}

vds::asymmetric_public_key vds::_storage_log::corresponding_public_key(
  const service_provider & sp,
  const principal_log_record & record)
{
  auto principal = this->principal_manager_->find_principal(sp, record.principal_id());
  if (!principal) {
    auto obj = std::dynamic_pointer_cast<json_object>(record.message());
    if (!obj) {
      throw std::runtime_error("Wrong messsage in the record");
    }

    std::string message_type;
    if (!obj->get_property("$t", message_type)) {
      throw std::runtime_error("Missing messsage type in the record");
    }

    if (server_log_root_certificate::message_type == message_type) {
      server_log_root_certificate msg(obj);
      return certificate::parse(msg.user_cert()).public_key();
    }

    throw std::runtime_error("Principial not found");
  }
  else {
    return certificate::parse(principal->cert_body()).public_key();
  }
}

void vds::_storage_log::validate_signature(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{

  std::string body = record.serialize(false)->str();
  if (!asymmetric_sign_verify::verify(
    hash::sha256(),
    this->corresponding_public_key(sp, record),
    signature,
    body.c_str(),
    body.length())) {
    sp.get<logger>()->trace(sp, "Wrong signature [%s] signed [%s]", body.c_str(), base64::from_bytes(signature).c_str());

    throw std::runtime_error("Invalid signature");
  }
}

void vds::_storage_log::create_database_objects(
  const service_provider & sp,
  uint64_t db_version,
  database_transaction & t)
{
  _principal_manager::create_database_objects(sp, db_version, t);
}

