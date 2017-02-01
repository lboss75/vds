#include "stdafx.h"
#include "vrt_json.h"

vds::vrt_json_value::vrt_json_value(const vrt_source_file * source_file, int line, int column)
  : source_file_(source_file), line_(line), column_(column)
{
}

vds::vrt_json_value::~vrt_json_value()
{
}

vds::vrt_json_primitive::vrt_json_primitive(const vrt_source_file * source_file, int line, int column, const std::string & value)
  : vrt_json_value(source_file, line, column), value_(value)
{
}

vds::vrt_json_object::vrt_json_object(const vrt_source_file * source_file, int line, int column)
  : vrt_json_value(source_file, line, column)
{
}

void vds::vrt_json_object::visit(const std::function<void(const vrt_json_property&)>& visitor) const
{
  for (const vrt_json_property & p : this->properties_) {
    visitor(p);
  }
}

vds::vrt_json_array::vrt_json_array(const vds::vrt_source_file* source_file, int line, int column)
: vrt_json_value(source_file, line, column)
{
}
