/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_database.h"
#include "server_database_p.h"
#include "principal_record.h"
#include "storage_log.h"
#include "chunk_manager_p.h"
#include "storage_log_p.h"

void vds::iserver_database::add_principal(
  const service_provider & sp,
  const principal_record & record)
{
  static_cast<_server_database *>(this)->add_principal(sp, record);
}

vds::guid vds::iserver_database::get_root_principal(const service_provider & sp)
{
  return static_cast<_server_database *>(this)->get_root_principal(sp);
}

void vds::iserver_database::add_user_principal(
  const service_provider & sp,
  const std::string & login,
  const principal_record & record)
{
  static_cast<_server_database *>(this)->add_user_principal(sp, login, record);
}


std::unique_ptr<vds::principal_record> vds::iserver_database::find_principal(
  const service_provider & sp,
  const guid & object_name)
{
  return static_cast<_server_database *>(this)->find_principal(
    sp,
    object_name);
}

std::unique_ptr<vds::principal_record> vds::iserver_database::find_user_principal(
  const service_provider & sp,
  const std::string & object_name)
{
  return static_cast<_server_database *>(this)->find_user_principal(
    sp,
    object_name);
}

void vds::iserver_database::add_object(
  const service_provider & sp,
  const principal_log_new_object & index)
{
  static_cast<_server_database *>(this)->add_object(sp, index);
}

void vds::iserver_database::add_endpoint(
  const service_provider & sp,
  const std::string& endpoint_id,
  const std::string& addresses)
{
  static_cast<_server_database *>(this)->add_endpoint(sp, endpoint_id, addresses);
}

void vds::iserver_database::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string>& addresses)
{
  static_cast<_server_database *>(this)->get_endpoints(sp, addresses);
}

vds::principal_log_record vds::iserver_database::add_local_record(
  const service_provider & sp,
  const principal_log_record::record_id & record_id,
  const guid & principal_id,
  const std::shared_ptr<json_value> & message,
  const vds::asymmetric_private_key & principal_private_key,
  const_data_buffer & signature)
{
  return static_cast<_server_database *>(this)->add_local_record(
    sp,
    record_id,
    principal_id,
    message,
    principal_private_key,
    signature);
}

size_t vds::iserver_database::get_current_state(
  const service_provider & sp, 
  std::list<guid> & active_records)
{
  return static_cast<_server_database *>(this)->get_current_state(sp, active_records);
}

bool vds::iserver_database::save_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  return static_cast<_server_database *>(this)->save_record(sp, record, signature);
}

void vds::iserver_database::get_unknown_records(
  const service_provider & sp,
  std::list<principal_log_record::record_id>& result)
{
  static_cast<_server_database *>(this)->get_unknown_records(sp, result);
}

bool vds::iserver_database::get_record(
  const service_provider & sp,
  const principal_log_record::record_id & id,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  return static_cast<_server_database *>(this)->get_record(sp, id, result_record, result_signature);
}

bool vds::iserver_database::get_front_record(
  const service_provider & sp,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  return static_cast<_server_database *>(this)->get_front_record(sp, result_record, result_signature);
}

void vds::iserver_database::processed_record(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  static_cast<_server_database *>(this)->processed_record(sp, id);
}

void vds::iserver_database::delete_record(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  static_cast<_server_database *>(this)->delete_record(sp, id);
}

vds::iserver_database::principal_log_state vds::iserver_database::get_record_state(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  return static_cast<_server_database *>(this)->principal_log_get_state(sp, id);
}

void vds::iserver_database::get_principal_log(
  const service_provider & sp,
  const guid & principal_id,
  size_t last_order_num,
  size_t & result_last_order_num,
  std::list<principal_log_record> & records)
{
  static_cast<_server_database *>(this)->get_principal_log(
    sp,
    principal_id,
    last_order_num,
    result_last_order_num,
    records);
}

////////////////////////////////////////////////////////
vds::_server_database::_server_database()
{
}

vds::_server_database::~_server_database()
{
}

void vds::_server_database::start(const service_provider & sp)
{
  //this->db_.start(sp);
  uint64_t db_version;

  filename db_filename(foldername(persistence::current_user(sp), ".vds"), "local.db");

  if (!file::exists(db_filename)) {
    db_version = 0;
    this->db_.open(db_filename);
  }
  else {
    this->db_.open(db_filename);
    
    auto t = this->db_.begin_transaction();
    auto st = t.parse("SELECT version FROM module WHERE id='kernel'");
    if(!st.execute()){
      throw std::runtime_error("Database has been corrupted");
    }
    
    st.get_value(0, db_version);
    this->db_.commit(t);
  }

  auto t = this->db_.begin_transaction();
  
  if (1 > db_version) {
    
    t.execute("CREATE TABLE module(\
    id VARCHAR(64) PRIMARY KEY NOT NULL,\
    version INTEGER NOT NULL,\
    installed DATETIME NOT NULL)");


    t.execute(
      "CREATE TABLE object(\
      object_id VARCHAR(64) NOT NULL,\
      length INTEGER NOT NULL,\
      meta_info BLOB NOT NULL,\
      CONSTRAINT pk_objects PRIMARY KEY (object_id))");

    t.execute(
      "CREATE TABLE endpoint(\
      endpoint_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      addresses TEXT NOT NULL)");
    
    t.execute(
      "CREATE TABLE network_route(\
      source_server_id VARCHAR(64) NOT NULL,\
      target_server_id VARCHAR(64) NOT NULL,\
      address VARCHAR(64) NOT NULL,\
      is_incomming BOOLEAN,\
      last_access DATETIME NOT NULL,\
      CONSTRAINT pk_network_route PRIMARY KEY (source_server_id,target_server_id,address,is_incomming))");

    t.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
    t.execute("INSERT INTO endpoint(endpoint_id, addresses) VALUES('default', 'udp://127.0.0.1:8050;https://127.0.0.1:8050')");
  }
  
  _storage_log::create_database_objects(sp, db_version, t);
  _chunk_manager::create_database_objects(sp, db_version, t);
  
  this->db_.commit(t);
}

void vds::_server_database::stop(const service_provider & sp)
{
  this->db_.close();
}


void vds::_server_database::add_object(
  const service_provider & sp,
  const principal_log_new_object & index)
{
  object_table t;
  database_transaction::current(sp)
  .execute(
    t.insert(t.object_id = index.index(), t.length = index.lenght(), t.meta_info = index.meta_data()));
}


void vds::_server_database::add_endpoint(
  const service_provider & sp,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  endpoint_table t;
  
  database_transaction::current(sp)
  .execute(
    t.insert(t.endpoint_id = endpoint_id, t.addresses = addresses));
}

void vds::_server_database::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string>& result)
{
  endpoint_table t;
  
  auto st = database_transaction::current(sp)
  .get_reader(
    t.select(t.endpoint_id, t.addresses));
  
  while(st.execute()){
    result.insert(std::pair<std::string, std::string>(t.endpoint_id.get(st), t.addresses.get(st)));
  }
}


/////////////////////////////////////////////


