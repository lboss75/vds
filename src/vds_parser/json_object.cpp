/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "json_object.h"

vds::json_value::json_value(int line, int column)
: line_(line), column_(column)
{
}

vds::json_value::~json_value()
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
  int line, int column
) : json_value(line, column)
{
}

void vds::json_object::visit(const std::function<void(const json_property&)>& visitor) const
{
  for (const json_property & property : this->properties_) {
    visitor(property);
  }
}

vds::json_array::json_array(int line, int column)
: json_value(line, column)
{

}
