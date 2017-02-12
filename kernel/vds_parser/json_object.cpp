/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "json_object.h"

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

void vds::json_value::escape(std::stringstream& stream, const std::string & value)
{
  stream << '\"';
  for(auto ch : value){
    if(isprint(ch)){
      stream << ch;
    }
    else {
      switch(ch){
        case '\"':
          stream << "\\\"";
          break;
        case '\n':
          stream << "\\n";
          break;
        case '\r':
          stream << "\\r";
          break;
        default:
          throw new std::runtime_error("Not implemented");
      }
    }
  }
  stream << '\"';
}

void vds::json_primitive::str(std::stringstream& stream) const
{
  escape(stream, this->value_);
}

void vds::json_property::str(std::stringstream& stream) const
{
  escape(stream, this->name_);
  stream << ':';
  if(nullptr == this->value_){
    stream << "null";
  }
  else {
    this->value_->str(stream);
  }
}

void vds::json_object::str(std::stringstream & stream) const
{
  stream << '{';
  bool bfirst = true;
  for(auto & p : this->properties_){
    if(!bfirst){
      stream << ',';
    }
    else {
      bfirst = false;
    }
    p->str(stream);
  }
  stream << '}';
}

void vds::json_array::str(std::stringstream& stream) const
{
  stream << '[';
  bool bfirst = true;
  for(auto & p : this->items_){
    if(!bfirst){
      stream << ',';
    }
    else {
      bfirst = false;
    }
    p->str(stream);
  }
  stream << ']';
}



