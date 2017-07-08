/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "database.h"
#include "database_p.h"

vds::database::database()
  : impl_(new _database())
{
}

vds::database::~database()
{
  delete this->impl_;
}

void vds::database::open(const filename & fn)
{
  this->impl_->open(fn);
}

void vds::database::close()
{
  this->impl_->close();
}

vds::database_transaction vds::database::begin_transaction()
{
  return vds::database_transaction(this->impl_->begin_transaction());
}

void vds::database::commit(vds::database_transaction& t)
{
  this->impl_->commit(t);
}

void vds::database::rollback(vds::database_transaction& t)
{
  this->impl_->rollback(t);
}

void vds::database_transaction::execute(const char * sql)
{
   this->impl_->execute(sql);
}

vds::sql_statement vds::database_transaction::parse(const char * sql)
{
  return this->impl_->parse(sql);
}

vds::sql_statement::sql_statement(_sql_statement * impl)
  : impl_(impl)
{
}

vds::sql_statement::sql_statement(sql_statement && original)
  : impl_(original.impl_)
{
  original.impl_ = nullptr;
}

vds::sql_statement::~sql_statement()
{
  delete this->impl_;
}

void vds::sql_statement::set_parameter(int index, int value)
{
  this->impl_->set_parameter(index, value);
}

void vds::sql_statement::set_parameter(int index, uint64_t value)
{
  this->impl_->set_parameter(index, value);
}

void vds::sql_statement::set_parameter(int index, const std::string & value)
{
  this->impl_->set_parameter(index, value);
}

void vds::sql_statement::set_parameter(int index, const guid & value)
{
  this->impl_->set_parameter(index, value);
}

void vds::sql_statement::set_parameter(int index, const const_data_buffer & value)
{
  this->impl_->set_parameter(index, value);
}

bool vds::sql_statement::execute()
{
  return this->impl_->execute();
}

bool vds::sql_statement::get_value(int index, int & value)
{
  return this->impl_->get_value(index, value);
}

bool vds::sql_statement::get_value(int index, uint64_t & value)
{
  return this->impl_->get_value(index, value);
}

bool vds::sql_statement::get_value(int index, std::string & value)
{
  return this->impl_->get_value(index, value);
}

bool vds::sql_statement::get_value(int index, guid & value)
{
  return this->impl_->get_value(index, value);
}

bool vds::sql_statement::get_value(int index, const_data_buffer & value)
{
  return this->impl_->get_value(index, value);
}

vds::sql_statement& vds::sql_statement::operator= (vds::sql_statement&& original)
{
  delete this->impl_;
  this->impl_ = original.impl_;
  original.impl_ = nullptr;
  
  return *this;
}
//////////////////////////////////////
class database_transaction_holder : public vds::service_provider::property_holder
{
public:
  database_transaction_holder(
    vds::database_transaction & t)
    : transaction(t)
  {
  }

  vds::database_transaction & transaction;
};

vds::database_transaction & vds::database_transaction::current(const service_provider & sp)
{
  auto holder = sp.get_property<database_transaction_holder>(service_provider::property_scope::any_scope);
  if (nullptr == holder) {
    throw std::runtime_error("Transaction is not openned");
  }

  return holder->transaction;
}

vds::database_transaction_scope::database_transaction_scope(const service_provider & sp, database & db)
  : db_(db), transaction_(db.begin_transaction()), successful_(false)
{
  sp.set_property(service_provider::property_scope::any_scope, new database_transaction_holder(this->transaction_));
}

vds::database_transaction_scope::~database_transaction_scope()
{
  if (!this->successful_) {
    this->db_.rollback(this->transaction_);
  }
}

void vds::database_transaction_scope::commit()
{
  this->db_.commit(this->transaction_);
  this->successful_ = true;
}
