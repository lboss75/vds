#include "stdafx.h"
#include "vrt_object.h"

vds::vrt_object::vrt_object(const vds::vrt_type* type)
: type_(type)
{
}

vds::vrt_string_object::vrt_string_object(const vrt_type * type, const std::string & value)
  : vrt_object(type), value_(value)
{
}
