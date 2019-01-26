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

vds::expected<bool> vds::json_object::get_property(const std::string & name, std::string & value) const
{
  auto value_obj = this->get_property(name);
  if (nullptr == value_obj) {
    return false;
  }

  auto pvalue = dynamic_cast<const json_primitive *>(value_obj.get());
  if (nullptr == pvalue) {
      return vds::make_unexpected<std::runtime_error>("Invalid property " + name + " type: expected string");
  }

  value = pvalue->value();
  return true;
}

vds::expected<bool> vds::json_object::get_property(const std::string& name, int& value) const
{
  std::string v;
  GET_EXPECTED(prop, this->get_property(name, v));
  if(!prop){
    return false;
  }
  
  value = std::atoi(v.c_str());
  return true;
}

vds::expected<bool> vds::json_object::get_property(const std::string& name, const_data_buffer& value) const
{
  std::string v;
  GET_EXPECTED(prop, this->get_property(name, v));
  if (!prop) {
    return false;
  }

  auto result = base64::to_bytes(v);
  if(result.has_error()) {
    return unexpected(std::move(result.error()));
  }

  value = result.value();
  return true;
}

vds::expected<bool> vds::json_object::get_property(const std::string & name, uint8_t & value) const
{
  std::string v;
  GET_EXPECTED(prop, this->get_property(name, v));
  if (!prop) {
    return false;
  }

  value = (uint8_t)std::atol(v.c_str());
  return true;
}

vds::expected<bool> vds::json_object::get_property(const std::string & name, uint16_t & value) const
{
  std::string v;
  GET_EXPECTED(prop, this->get_property(name, v));
  if (!prop) {
    return false;
  }

  value = (uint16_t)std::atol(v.c_str());
  return true;
}

vds::expected<bool> vds::json_object::get_property(const std::string & name, uint32_t & value) const
{
  std::string v;
  GET_EXPECTED(prop, this->get_property(name, v));
  if (!prop) {
    return false;
  }

  value = (uint32_t)std::atol(v.c_str());
  return true;
}

vds::expected<bool> vds::json_object::get_property(const std::string & name, uint64_t & value) const
{
  std::string v;
  GET_EXPECTED(prop, this->get_property(name, v));
  if (!prop) {
    return false;
  }

  value = (uint64_t)std::atol(v.c_str());
  return true;
}

vds::expected<bool> vds::json_object::get_property(const std::string& name, std::list< const_data_buffer >& value) const
{
  auto array = std::dynamic_pointer_cast<json_array>(this->get_property(name));
  if(!array){
      return vds::make_unexpected<std::runtime_error>("Invalid property " + name);
  }
  
  for(size_t i = 0; i < array->size(); ++i) {
    auto item = std::dynamic_pointer_cast<json_primitive>(array->get(i));
    if(item){
      GET_EXPECTED(v, base64::to_bytes(item->value()));
      value.push_back(v);
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

  this->add_property(std::make_shared<json_property>(name, std::make_shared<json_primitive>(std::to_string(value))));
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

vds::expected<std::string> vds::json_value::str() const
{
  json_writer writer;
  CHECK_EXPECTED(this->str(writer));
  return writer.str();
}

vds::expected<void> vds::json_primitive::str(json_writer & writer) const
{
  return writer.write_string_value(this->value_);
}

std::shared_ptr<vds::json_value> vds::json_primitive::clone(bool /*is_deep*/) const
{
  return std::make_shared<json_primitive>(this->value_);
}

vds::expected<void> vds::json_property::str(json_writer & writer) const
{
  CHECK_EXPECTED(writer.start_property(this->name_));
  if(nullptr == this->value_){
    CHECK_EXPECTED(writer.write_null_value());
  }
  else {
    CHECK_EXPECTED(this->value_->str(writer));
  }

  return writer.end_property();
}

std::shared_ptr<vds::json_value> vds::json_property::clone(bool is_deep) const
{
  return std::shared_ptr<json_value>(
    std::make_shared<json_property>(
      this->name_,
      (is_deep && this->value_) ? this->value_->clone(true) : this->value_));
}

vds::expected<void> vds::json_object::str(json_writer & writer) const
{
  CHECK_EXPECTED(writer.start_object());
  for(auto & p : this->properties_){
    CHECK_EXPECTED(p->str(writer));
  }

  return writer.end_object();
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

vds::expected<void> vds::json_array::str(json_writer & writer) const
{
  CHECK_EXPECTED(writer.start_array());

  for(auto & p : this->items_){
    CHECK_EXPECTED(p->str(writer));
  }

  return writer.end_array();
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



