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
  this->stop();
}

void vds::server_database::add_cert(const cert & record)
{
  this->impl_->add_cert(record);
}

std::unique_ptr<vds::cert> vds::server_database::find_cert(const std::string & object_name) const
{
  return this->impl_->find_cert(object_name);
}

void vds::server_database::add_object(
  const guid & server_id,
  uint64_t index,
  const data_buffer & signature)
{
  this->impl_->add_object(server_id, index, signature);
}

uint64_t vds::server_database::last_object_index(const guid& server_id)
{
  return this->impl_->last_object_index(server_id);
}

void vds::server_database::add_endpoint(const std::string& endpoint_id, const std::string& addresses)
{
  this->impl_->add_endpoint(endpoint_id, addresses);
}

void vds::server_database::get_endpoints(std::map<std::string, std::string>& addresses)
{
  this->impl_->get_endpoints(addresses);
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
      "CREATE TABLE objects(\
      server_id VARCHAR(64) NOT NULL,\
      object_index INTEGER NOT NULL,\
      signature VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_objects PRIMARY KEY (server_id, object_index, signature))");

    this->db_.execute(
      "CREATE TABLE cert(\
      object_name VARCHAR(64) PRIMARY KEY NOT NULL,\
      source_server_id VARCHAR(64) NOT NULL,\
      object_index INTEGER NOT NULL,\
      signature VARCHAR(64) NOT NULL,\
      password_hash VARCHAR(64) NOT NULL)");
    
    this->db_.execute(
      "CREATE TABLE endpoints(\
      endpoint_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      addresses TEXT NOT NULL)");

    this->db_.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
    this->db_.execute("INSERT INTO endpoints(iendpoint_id, addresses) VALUES('default', 'udp://127.0.0.1:8050,https://127.0.0.1:8050')");
  }
}

void vds::_server_database::add_cert(const cert & record)
{
  this->add_cert_statement_.execute(
    this->db_,
    "INSERT INTO cert(object_name, source_server_id, object_index, signature, password_hash)\
      VALUES (@object_name, @source_server_id, @object_index, @signature, @password_hash)",

    record.object_name(),
    record.object_id().source_server_id(),
    record.object_id().index(),
    record.object_id().signature(),
    record.password_hash());
}

std::unique_ptr<vds::cert> vds::_server_database::find_cert(const std::string & object_name)
{
  std::unique_ptr<cert> result;
  this->find_cert_query_.query(
    this->db_,
    "SELECT source_server_id, object_index, signature, password_hash\
     FROM cert\
     WHERE object_name=@object_name",
    [&result, object_name](sql_statement & st)->bool{
      
      guid source_id;
      uint64_t index;
      data_buffer signature;
      data_buffer password_hash;
      
      st.get_value(0, source_id);
      st.get_value(1, index);
      st.get_value(2, signature);
      st.get_value(3, password_hash);

      result.reset(new cert(
        object_name,
        full_storage_object_id(source_id, index, signature),
        password_hash));
      
      return false; },
    object_name);
  
  return result;
}

void vds::_server_database::add_object(
  const guid & server_id,
  uint64_t index,
  const data_buffer & signature)
{
  this->add_object_statement_.execute(
    this->db_,
    "INSERT INTO objects(server_id, object_index, signature) VALUES (@server_id, @object_index, @signature)",
    server_id,
    index,
    signature);
}

uint64_t vds::_server_database::last_object_index(const guid& server_id)
{
  uint64_t result = 0;
  this->last_object_index_query_.query(
    this->db_,
    "SELECT MAX(object_index) FROM objects WHERE server_id=@server_id",
    [&result](sql_statement & st)->bool{
      st.get_value(0, result);
      return false;
    },
    server_id);
  
  return result;
}
void vds::_server_database::add_endpoints(
  const std::string & endpoint_id,
  const std::string & addresses)
{
  this->add_endpoint_statement_.execute(
    this->db_,
    "INSERT INTO endpoints(endpoint_id, addresses) VALUES (@endpoint_id, @addresses)",
    endpoint_id,
    addresses);
}

void vds::_server_database::get_endpoints(std::map<std::string, std::string>& result)
{
  this->last_object_index_query_.query(
    this->db_,
    "SELECT endpoint_id, addresses FROM endpoints",
    [&result](sql_statement & st)->bool{
      std::string endpoint_id;
      std::string addresses;
      st.get_value(0, endpoint_id);
      st.get_value(1, addresses);
      return true;
    });
}
