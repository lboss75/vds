/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "json_object.h"
#include "json_writer.h"

vds::json_value::json_value()
: line_(-1), column_(-1)
{
}

vds::json_value::json_value(int line, int column)
: line_(line), column_(column)
{
}

vds::json_value::~json_value()
{
}

vds::json_primitive::json_primitive(
    const std::string & value)
: value_(value)
{
}

vds::json_primitive::json_primitive(
    int line,
    int column,
    const std::string & value)
: json_value(line, column), value_(value)
{
}


vds::json_object::json_object(
)
{
}

vds::json_object::json_object(
  int line, int column
) : json_value(line, column)
{
}

void vds::json_object::visit(const std::function<void(const json_property&)>& visitor) const
{
  for (const auto & property : this->properties_) {
    visitor(*property.get());
  }
}

const vds::json_value * vds::json_object::get_property(const std::string & name) const
{
  for (const auto & property : this->properties_) {
    if (property->name() == name) {
      return property->value();
    }
  }

  return nullptr;
}

void vds::json_object::add_property(json_property * prop)
{
  this->properties_.push_back(std::unique_ptr<json_property>(prop));
}

vds::json_array::json_array()
{
}

vds::json_array::json_array(int line, int column)
: json_value(line, column)
{

}

vds::json_property::json_property(
  const std::string& name, vds::json_value* val)
: name_(name), value_(val)
{
}

vds::json_property::json_property(int line, int column)
  : json_value(line, column)
{
}

std::string vds::json_value::str() const
{
  json_writer writer;
  this->str(writer);
  return writer.str();
}

void vds::json_primitive::str(json_writer & writer) const
{
  writer.write_string_value(this->value_);
}

void vds::json_property::str(json_writer & writer) const
{
  writer.start_property(this->name_);
  if(nullptr == this->value_){
    writer.write_null_value();
  }
  else {
    this->value_->str(writer);
  }

  writer.end_property();
}

void vds::json_object::str(json_writer & writer) const
{
  writer.start_object();
  for(auto & p : this->properties_){
    p->str(writer);
  }

  writer.end_object();
}

void vds::json_array::str(json_writer & writer) const
{
  writer.start_array();

  for(auto & p : this->items_){
    p->str(writer);
  }

  writer.end_array();
}



