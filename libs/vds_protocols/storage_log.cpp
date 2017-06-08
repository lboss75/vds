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
  const std::shared_ptr<json_value> & record)
{
  static_cast<_storage_log *>(this)->add_to_local_log(sp, principal_id, record);
}

size_t vds::istorage_log::new_message_id()
{
  return static_cast<_storage_log *>(this)->new_message_id();
}

vds::async_task<> vds::istorage_log::register_server(
  const service_provider & sp,
  const std::string & server_certificate)
{
  return static_cast<_storage_log *>(this)->register_server(sp, server_certificate);
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
  const const_data_buffer & signature,
  bool check_signature /*= true*/)
{
  static_cast<_storage_log *>(this)->apply_record(
    sp,
    record,
    signature,
    check_signature);
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
  
  hash ph(hash::sha256());
  ph.update(password.c_str(), password.length());
  ph.final();
  
  this->add_to_local_log(
    sp,
    principal_id,
    server_log_root_certificate(
      principal_id,
      root_certificate.str(),
      private_key.str(password),
      ph.signature()).serialize());
  this->add_to_local_log(sp, principal_id, server_log_new_server(
    this->current_server_id_,
    principal_id,
    this->server_certificate_.str(),
    this->current_server_key_.str(password),
    ph.signature()).serialize());
  this->add_to_local_log(sp, principal_id, server_log_new_endpoint(this->current_server_id_, addresses).serialize());
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
  const std::string & server_certificate)
{
  return create_async_task(
    [this, server_certificate](
      const std::function<void(const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp) {
    throw std::runtime_error("Not implemented");
    //this->add_to_local_log(sp, server_log_new_server(server_certificate).serialize());
    done(sp);
  });
}

void vds::_storage_log::add_to_local_log(
  const service_provider & sp,
  const guid & principal_id,
  const std::shared_ptr<json_value> & message)
{
  std::lock_guard<std::mutex> lock(this->record_state_mutex_);

  const_data_buffer signature;
  auto result = sp.get<iserver_database>()->add_local_record(
      sp,
      guid::new_guid(),
      principal_id,
      message,
      signature);
    
  this->apply_record(sp, result, signature);

  auto sl = sp.get<_server_log_sync>(false);
  if (nullptr != sl) {
    sl->on_new_local_record(sp, result, signature);
  }
}

void vds::_storage_log::apply_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature,
  bool check_signature /*= true*/)
{
  sp.get<logger>()->debug(sp, "Apply record %s", record.id().str().c_str());
  auto state = sp.get<iserver_database>()->get_record_state(sp, record.id());
  if(iserver_database::principal_log_state::front != state){
    throw std::runtime_error("Invalid server state");
  }
 
  auto obj = std::dynamic_pointer_cast<json_object>(record.message());
  if(!obj){
    sp.get<logger>()->info(sp, "Wrong messsage in the record %s", record.id().str().c_str());
    sp.get<iserver_database>()->delete_record(sp, record.id());
    return;
  }
  
  try {
    std::string message_type;
    if (!obj->get_property("$t", message_type)) {
      sp.get<logger>()->info(sp, "Missing messsage type in the record %s", record.id().str().c_str());
      sp.get<iserver_database>()->delete_record(sp, record.id());
      return;
    }
    
    if (principal_log_new_object::message_type == message_type) {
      principal_log_new_object msg(obj);
      
      sp.get<logger>()->debug(sp, "Add object %s",
        record.id().str().c_str(),
        msg.index());

      sp.get<iserver_database>()->add_object(sp, msg);
    }
    else if(server_log_root_certificate::message_type == message_type){
      server_log_root_certificate msg(obj);
      
      sp.get<iserver_database>()->add_user_principal(
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

      sp.get<iserver_database>()->add_principal(
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
      sp.get<logger>()->info(sp, "Enexpected messsage type '%s' in the record %s",
        message_type.c_str(),
        record.id().str().c_str());

      sp.get<iserver_database>()->delete_record(sp, record.id());
      return;
    }

    this->last_applied_record_ = record.id();
  }
  catch(...){
    sp.get<logger>()->info(sp, "%s", exception_what(std::current_exception()).c_str());
    sp.get<iserver_database>()->delete_record(sp, record.id());
    return;
  }
  
  sp.get<iserver_database>()->processed_record(sp, record.id());
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

  principal_log_record record;
  const_data_buffer signature;
  while(sp.get<iserver_database>()->get_front_record(sp, record, signature)){
    this->apply_record(sp, record, signature);
  }

  return true;
}

