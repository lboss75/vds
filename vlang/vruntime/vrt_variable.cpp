#include "stdafx.h"
#include "vrt_variable.h"

vds::vrt_variable::vrt_variable(
  const vrt_variable_declaration * declaration,
  const std::shared_ptr<vrt_object>& value)
  : declaration_(declaration), value_(value)
{
}
