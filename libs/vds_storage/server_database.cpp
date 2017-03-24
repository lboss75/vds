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
  int db_version;

  filename db_filename(foldername(persistence::current_user(this->sp_), ".vds"), "local.db");

  if (!file::exists(db_filename)) {
    db_version = 0;
    this->db_.open(db_filename);
  }
  else {
    this->db_.open(db_filename);
  }


  if (1 > db_version) {
    this->db_.execute("CREATE TABLE module(\
    id VARCHAR(64) PRIMARY KEY NOT NULL,\
    version INTEGER NOT NULL,\
    installed DATETIME NOT NULL)");

    this->db_.execute(
      "CREATE TABLE cert(\
      object_name VARCHAR(64) PRIMARY KEY NOT NULL,\
      source_server_id VARCHAR(64) NOT NULL,\
      object_index INTEGER NOT NULL,\
      signature VARCHAR(64) NOT NULL,\
      password_hash VARCHAR(64) NOT NULL)");

    this->db_.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
  }
}

void vds::_server_database::add_cert(const cert & record)
{
  if (!this->add_cert_statement_) {
    this->add_cert_statement_.reset(
      new sql_statement(
        this->db_.parse(
          "INSERT INTO cert(object_name, source_server_id, object_index, signature, password_hash)\
           VALUES (?object_name, ?source_server_id, ?object_index, ?signature, ?password_hash)")));
  }

  this->add_cert_statement_->set_parameter(0, record.object_name());
  this->add_cert_statement_->set_parameter(1, record.object_id().source_server_id());
  this->add_cert_statement_->set_parameter(2, record.object_id().index());
  this->add_cert_statement_->set_parameter(3, record.object_id().signature());
  this->add_cert_statement_->set_parameter(4, record.password_hash());

}
