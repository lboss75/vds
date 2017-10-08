/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "database.h"
#include "private/database_p.h"

vds::database::database()
  : impl_(new _database())
{
}

vds::database::~database()
{
  delete this->impl_;
}

void vds::database::open(const service_provider & sp, const filename & fn)
{
  this->impl_->open(sp, fn);
}

void vds::database::close()
{
  this->impl_->close();
}

void vds::database::async_transaction(
  const service_provider & sp,
  const std::function<bool(database_transaction & tr)> & callback)
{
  this->impl_->async_transaction(sp, callback);
}

void vds::database::sync_transaction(
  const service_provider & sp,
  const std::function<bool(database_transaction & tr)> & callback)
{
  this->impl_->sync_transaction(sp, callback);
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

bool vds::sql_statement::get_value(int index, double & value)
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
