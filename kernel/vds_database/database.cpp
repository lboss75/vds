/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "database.h"
#include "database_p.h"

vds::database::database(const service_provider & sp)
  : impl_(new _database(sp, this))
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

void vds::database::execute(const std::string & sql)
{
  this->impl_->execute(sql);
}

vds::sql_statement vds::database::parse(const std::string & sql)
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

void vds::sql_statement::set_parameter(int index, const std::string & value)
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

bool vds::sql_statement::get_value(int index, std::string & value)
{
  return this->impl_->get_value(index, value);
}
