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

void vds::json_object::visit(const std::function<void(const std::shared_ptr<json_property> & )>& visitor) const
{
  for (const auto & property : this->properties_) {
    visitor(property);
  }
}

std::shared_ptr<vds::json_value> vds::json_object::get_property(const std::string & name) const
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

  auto pvalue = dynamic_cast<const json_primitive *>(value_obj.get());
  if (nullptr == pvalue) {
    if (throw_error) {
      throw std::runtime_error("Invalid property " + name + " type: expected string");
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

bool vds::json_object::get_property(const std::string& name, const_data_buffer& value, bool throw_error) const
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

bool vds::json_object::get_property(const std::string& name, std::list< const_data_buffer >& value, bool throw_error) const
{
  auto array = std::dynamic_pointer_cast<json_array>(this->get_property(name));
  if(!array){
    if(throw_error){
      throw std::runtime_error("Invalid property " + name);
    }
    
    return false;
  }
  
  for(size_t i = 0; i < array->size(); ++i) {
    auto item = std::dynamic_pointer_cast<json_primitive>(array->get(i));
    if(item){
      value.push_back(base64::to_bytes(item->value()));
    }
  }
  
  return true;

}

void vds::json_object::add_property(const std::shared_ptr<json_property> & prop)
{
  this->properties_.push_back(prop);
}

void vds::json_object::add_property(const std::string & name, const std::shared_ptr<json_value> & value)
{
  this->add_property(std::make_shared<json_property>(name, value));
}

void vds::json_object::add_property(const std::string & name, uint64_t value)
{
  this->add_property(std::make_shared<json_property>(name, std::make_shared<json_primitive>(std::to_string(value))));
}

void vds::json_object::add_property(const std::string& name, const std::chrono::system_clock::time_point& value) {

  this->add_property(std::make_shared<json_property>(name, std::make_shared<json_primitive>(vds::to_string(value))));
}

void vds::json_object::add_property(const std::string & name, const std::string & value)
{
  this->add_property(std::make_shared<json_property>(name, std::make_shared<json_primitive>(value)));
}

void vds::json_object::add_property(const std::string& name, const const_data_buffer& value)
{
  this->add_property(name, base64::from_bytes(value.data(), value.size()));
}

void vds::json_object::add_property(const std::string & name, const std::list<const_data_buffer> & value)
{
  auto array = std::make_shared<json_array>();
  for(auto & item : value){
    array->add(std::make_shared<json_primitive>(base64::from_bytes(item)));
  }
  
  this->add_property(name, array);
}

vds::json_array::json_array()
{
}

vds::json_array::json_array(int line, int column)
: json_value(line, column)
{

}

vds::json_property::json_property(
  const std::string& name,
  const std::shared_ptr<vds::json_value> & val)
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

std::shared_ptr<vds::json_value> vds::json_primitive::clone(bool /*is_deep*/) const
{
  return std::shared_ptr<json_value>(new json_primitive(this->value_));
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

std::shared_ptr<vds::json_value> vds::json_property::clone(bool is_deep) const
{
  return std::shared_ptr<json_value>(
    std::make_shared<json_property>(
      this->name_,
      (is_deep && this->value_) ? this->value_->clone(true) : this->value_));
}

void vds::json_object::str(json_writer & writer) const
{
  writer.start_object();
  for(auto & p : this->properties_){
    p->str(writer);
  }

  writer.end_object();
}

std::shared_ptr<vds::json_value> vds::json_object::clone(bool is_deep) const
{
  auto s = std::make_shared<json_object>();

  for (auto& p : this->properties_) {
    if(is_deep){
      s->add_property(std::static_pointer_cast<json_property>(p->clone(true)));
    }
    else {
      s->add_property(p);
    }
  }

  return s;
}

void vds::json_array::str(json_writer & writer) const
{
  writer.start_array();

  for(auto & p : this->items_){
    p->str(writer);
  }

  writer.end_array();
}

std::shared_ptr<vds::json_value> vds::json_array::clone(bool is_deep) const
{
  std::shared_ptr<json_value> s(new json_array());

  for (auto& i : this->items_) {
    if(is_deep){
      static_cast<json_array *>(s.get())->add(i->clone(true));
    }
    else {
      static_cast<json_array *>(s.get())->add(i);
    }
  }

  return s;
}



