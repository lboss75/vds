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

bool vds::json_object::get_property(const std::string & name, std::string & value, bool throw_error) const
{
  auto value_obj = this->get_property(name);
  if (nullptr == value_obj) {
    return false;
  }

  auto pvalue = dynamic_cast<const json_primitive *>(value_obj);
  if (nullptr == pvalue) {
    if (throw_error) {
      throw new std::runtime_error("Invalid property " + name + " type: expected string");
    }

    return false;
  }

  value = pvalue->value();
  return true;
}

bool vds::json_object::get_property(const std::string& name, int& value, bool throw_error) const
{
  std::string v;
  if(!this->get_property(name, v, throw_error)){
    return false;
  }
  
  value = std::atoi(v.c_str());
  return true;
}

//bool vds::json_object::get_property(const std::string & name, size_t & value, bool throw_error) const
//{
//  std::string v;
//  if (!this->get_property(name, v, throw_error)) {
//    return false;
//  }
//
//  value = (size_t)std::atol(v.c_str());
//  return true;
//}

bool vds::json_object::get_property(const std::string& name, data_buffer& value, bool throw_error) const
{
  std::string v;
  if (!this->get_property(name, v, throw_error)) {
    return false;
  }

  value = base64::to_bytes(v);
  return true;
}

bool vds::json_object::get_property(const std::string & name, uint8_t & value, bool throw_error) const
{
  std::string v;
  if (!this->get_property(name, v, throw_error)) {
    return false;
  }

  value = (uint8_t)std::atol(v.c_str());
  return true;
}

bool vds::json_object::get_property(const std::string & name, uint16_t & value, bool throw_error) const
{
  std::string v;
  if (!this->get_property(name, v, throw_error)) {
    return false;
  }

  value = (uint16_t)std::atol(v.c_str());
  return true;
}

bool vds::json_object::get_property(const std::string & name, uint32_t & value, bool throw_error) const
{
  std::string v;
  if (!this->get_property(name, v, throw_error)) {
    return false;
  }

  value = (uint32_t)std::atol(v.c_str());
  return true;
}

bool vds::json_object::get_property(const std::string & name, uint64_t & value, bool throw_error) const
{
  std::string v;
  if (!this->get_property(name, v, throw_error)) {
    return false;
  }

  value = (uint64_t)std::atol(v.c_str());
  return true;
}


void vds::json_object::add_property(json_property * prop)
{
  this->properties_.push_back(std::unique_ptr<json_property>(prop));
}

void vds::json_object::add_property(const std::string & name, uint64_t value)
{
  this->add_property(new json_property(name, new json_primitive(std::to_string(value))));
}

void vds::json_object::add_property(const std::string & name, const std::string & value)
{
  this->add_property(new json_property(name, new json_primitive(value)));
}

void vds::json_object::add_property(const std::string& name, const data_buffer& value)
{
  this->add_property(name, base64::from_bytes(value.data(), value.size()));
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

std::unique_ptr<vds::json_value> vds::json_primitive::clone() const
{
  return std::unique_ptr<json_value>(new json_primitive(this->value_));
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

std::unique_ptr<vds::json_value> vds::json_property::clone() const
{
  return std::unique_ptr<json_value>(
    new json_property(
      this->name_,
      (nullptr == this->value_) ? nullptr : this->value_->clone().release()));
}

void vds::json_object::str(json_writer & writer) const
{
  writer.start_object();
  for(auto & p : this->properties_){
    p->str(writer);
  }

  writer.end_object();
}

std::unique_ptr<vds::json_value> vds::json_object::clone() const
{
  std::unique_ptr<json_object> s(new json_object());

  for (auto& p : this->properties_) {
    s->add_property(static_cast<json_property *>(p->clone().release()));
  }

  return std::unique_ptr<json_value>(s.release());
}

void vds::json_array::str(json_writer & writer) const
{
  writer.start_array();

  for(auto & p : this->items_){
    p->str(writer);
  }

  writer.end_array();
}

std::unique_ptr<vds::json_value> vds::json_array::clone() const
{
  std::unique_ptr<json_array> s(new json_array());

  for (auto& i : this->items_) {
    s->add(i->clone().release());
  }

  return std::unique_ptr<json_value>(s.release());
}



