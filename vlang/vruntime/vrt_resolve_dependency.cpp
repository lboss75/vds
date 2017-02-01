#include "stdafx.h"
#include "vrt_resolve_dependency.h"
#include "vrt_context.h"

vds::vrt_resolve_dependency::vrt_resolve_dependency(
  const vrt_source_file * file,
  int line,
  int column,
  const vrt_type * interface_type)
  : vrt_statement(file, line, column), interface_type_(interface_type)
{
}

bool vds::vrt_resolve_dependency::execute(vrt_context & context) const
{
  return context.resolve_dependency(this->interface_type_);
}
