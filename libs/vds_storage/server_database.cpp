/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_database.h"
#include "server_database_p.h"
#include "cert.h"

vds::server_database::server_database(const service_provider & sp)
  : impl_(new _server_database(sp, this))
{
}

vds::server_database::~server_database()
{
  delete this->impl_;
}

void vds::server_database::start()
{
  this->impl_->start();
}

void vds::server_database::stop()
{
  this->impl_->stop();
}

vds::iserver_database::iserver_database(server_database * owner)
  : owner_(owner)
{
}

void vds::iserver_database::add_cert(const cert & record)
{
  this->owner_->impl_->add_cert(record);
}

std::unique_ptr<vds::cert> vds::iserver_database::find_cert(const std::string & object_name) const
{
  return this->owner_->impl_->find_cert(object_name);
}

void vds::iserver_database::add_object(
  const guid & server_id,
  const server_log_new_object & index)
{
  this->owner_->impl_->add_object(server_id, index);
}

void vds::iserver_database::add_file(
  const guid & server_id,
  const server_log_file_map & fm)
{
  this->owner_->impl_->add_file(server_id, fm);
}

uint64_t vds::iserver_database::last_object_index(const guid& server_id)
{
  return this->owner_->impl_->last_object_index(server_id);
}

void vds::iserver_database::add_endpoint(const std::string& endpoint_id, const std::string& addresses)
{
  this->owner_->impl_->add_endpoint(endpoint_id, addresses);
}

void vds::iserver_database::get_endpoints(std::map<std::string, std::string>& addresses)
{
  this->owner_->impl_->get_endpoints(addresses);
}
////////////////////////////////////////////////////////
vds::_server_database::_server_database(const service_provider & sp, server_database * owner)
  : sp_(sp),
  owner_(owner),
  db_(sp)
{
}

vds::_server_database::~_server_database()
{
}

void vds::_server_database::start()
{
  uint64_t db_version;

  filename db_filename(foldername(persistence::current_user(this->sp_), ".vds"), "local.db");

  if (!file::exists(db_filename)) {
    db_version = 0;
    this->db_.open(db_filename);
  }
  else {
    this->db_.open(db_filename);
    
    auto st = this->db_.parse("SELECT version FROM module WHERE id='kernel'");
    if(!st.execute()){
      throw new std::runtime_error("Database has been corrupted");
    }
    
    st.get_value(0, db_version);
  }


  if (1 > db_version) {
    this->db_.execute("CREATE TABLE module(\
    id VARCHAR(64) PRIMARY KEY NOT NULL,\
    version INTEGER NOT NULL,\
    installed DATETIME NOT NULL)");

    this->db_.execute(
      "CREATE TABLE object(\
      server_id VARCHAR(64) NOT NULL,\
      object_index INTEGER NOT NULL,\
      original_lenght INTEGER NOT NULL,\
      original_hash VARCHAR(64) NOT NULL,\
      target_lenght INTEGER NOT NULL, \
      target_hash VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_objects PRIMARY KEY (server_id, object_index))");

    this->db_.execute(
      "CREATE TABLE cert(\
      object_name VARCHAR(64) PRIMARY KEY NOT NULL,\
      source_server_id VARCHAR(64) NOT NULL,\
      object_index INTEGER NOT NULL,\
      password_hash VARCHAR(64) NOT NULL)");
    
    this->db_.execute(
      "CREATE TABLE endpoint(\
      endpoint_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      addresses TEXT NOT NULL)");

    this->db_.execute(
      "CREATE TABLE file(\
      version_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      server_id VARCHAR(64) NOT NULL,\
      user_login VARCHAR(64) NOT NULL,\
      name TEXT NOT NULL)");
    
    this->db_.execute(
      "CREATE TABLE file_map(\
      version_id VARCHAR(64) NOT NULL,\
      object_index INTEGER NOT NULL,\
      CONSTRAINT pk_file_map PRIMARY KEY (version_id, object_index))");
    
    this->db_.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
    this->db_.execute("INSERT INTO endpoint(endpoint_id, addresses) VALUES('default', 'udp://127.0.0.1:8050,https://127.0.0.1:8050')");
  }
}

void vds::_server_database::stop()
{
  this->db_.close();
}

void vds::_server_database::add_cert(const cert & record)
{
  this->add_cert_statement_.execute(
    this->db_,
    "INSERT INTO cert(object_name, source_server_id, object_index, password_hash)\
      VALUES (@object_name, @source_server_id, @object_index, @password_hash)",

    record.object_name(),
    record.object_id().source_server_id(),
    record.object_id().index(),
    record.password_hash());
}

std::unique_ptr<vds::cert> vds::_server_database::find_cert(const std::string & object_name)
{
  std::unique_ptr<cert> result;
  this->find_cert_query_.query(
    this->db_,
    "SELECT source_server_id, object_index, password_hash\
     FROM cert\
     WHERE object_name=@object_name",
    [&result, object_name](sql_statement & st)->bool{
      
      guid source_id;
      uint64_t index;
      data_buffer password_hash;
      
      st.get_value(0, source_id);
      st.get_value(1, index);
      st.get_value(2, password_hash);

      result.reset(new cert(
        object_name,
        full_storage_object_id(source_id, index),
        password_hash));
      
      return false; },
    object_name);
  
  return result;
}

void vds::_server_database::add_object(
  const guid & server_id,
  const server_log_new_object & index)
{
  this->add_object_statement_.execute(
    this->db_,
    "INSERT INTO object(server_id, object_index, original_lenght, original_hash, target_lenght, target_hash)\
    VALUES (@server_id, @object_index, @original_lenght, @original_hash, @target_lenght, @target_hash)",
    server_id,
    index.index(),
    index.original_lenght(),
    index.original_hash(),
    index.target_lenght(),
    index.target_hash());
}

uint64_t vds::_server_database::last_object_index(const guid& server_id)
{
  uint64_t result = 0;
  this->last_object_index_query_.query(
    this->db_,
    "SELECT MAX(object_index)+1 FROM object WHERE server_id=@server_id",
    [&result](sql_statement & st)->bool{
      st.get_value(0, result);
      return false;
    },
    server_id);
  
  return result;
}

void vds::_server_database::add_endpoint(
  const std::string & endpoint_id,
  const std::string & addresses)
{
  this->add_endpoint_statement_.execute(
    this->db_,
    "INSERT INTO endpoint(endpoint_id, addresses) VALUES (@endpoint_id, @addresses)",
    endpoint_id,
    addresses);
}

void vds::_server_database::get_endpoints(std::map<std::string, std::string>& result)
{
  this->get_endpoints_query_.query(
    this->db_,
    "SELECT endpoint_id, addresses FROM endpoint",
    [&result](sql_statement & st)->bool{
      std::string endpoint_id;
      std::string addresses;
      st.get_value(0, endpoint_id);
      st.get_value(1, addresses);

      result.insert(std::pair<std::string, std::string>(endpoint_id, addresses));
      return true;
    });
}

void vds::_server_database::add_file(
  const guid & server_id,
  const server_log_file_map & fm)
{
    this->add_file_statement_.execute(
      this->db_,
      "INSERT INTO file(version_id,server_id,user_login,name)\
      VALUES(@version_id,@server_id,@user_login,@name)",
      fm.version_id(),
      server_id,
      fm.user_login(),
      fm.name());
    
    for(auto & item : fm.items()){
      this->add_file_map_statement_.execute(
        this->db_,
        "INSERT INTO file_map(version_id,object_index)\
        VALUES(@version_id,@object_index)",
        fm.version_id(),
        item.index());
    }
}