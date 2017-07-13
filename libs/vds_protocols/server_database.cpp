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


void vds::iserver_database::add_object(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_new_object & index)
{
  static_cast<_server_database *>(this)->add_object(sp, tr, index);
}

void vds::iserver_database::add_endpoint(
  const service_provider & sp,
  database_transaction & tr,
  const std::string& endpoint_id,
  const std::string& addresses)
{
  static_cast<_server_database *>(this)->add_endpoint(sp, tr, endpoint_id, addresses);
}

void vds::iserver_database::get_endpoints(
  const service_provider & sp,
  database_transaction & tr,
  std::map<std::string, std::string>& addresses)
{
  static_cast<_server_database *>(this)->get_endpoints(sp, tr, addresses);
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
    
    auto t = this->db_.begin_transaction(sp);
    auto st = t.parse("SELECT version FROM module WHERE id='kernel'");
    if(!st.execute()){
      throw std::runtime_error("Database has been corrupted");
    }
    
    st.get_value(0, db_version);
    this->db_.commit(t);
  }

  auto t = this->db_.begin_transaction(sp);
  
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
  database_transaction & tr,
  const principal_log_new_object & index)
{
  object_table t;
  tr.execute(
    t.insert(t.object_id = index.index(), t.length = index.lenght(), t.meta_info = index.meta_data()));
}


void vds::_server_database::add_endpoint(
  const service_provider & sp,
  database_transaction & tr,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  endpoint_table t;
  
  tr.execute(
    t.insert(t.endpoint_id = endpoint_id, t.addresses = addresses));
}

void vds::_server_database::get_endpoints(
  const service_provider & sp,
  database_transaction & tr,
  std::map<std::string, std::string>& result)
{
  endpoint_table t;
  
  auto st = tr.get_reader(
    t.select(t.endpoint_id, t.addresses));
  
  while(st.execute()){
    result.insert(std::pair<std::string, std::string>(t.endpoint_id.get(st), t.addresses.get(st)));
  }
}


/////////////////////////////////////////////


