#include "stdafx.h"
#include "vrt_variable_declaration.h"
#include "vrt_package.h"
#include "vrt_statement.h"

vds::vrt_variable_declaration::vrt_variable_declaration(
  const vrt_source_file * file,
  int line,
  int column,
  const vrt_type * type,
  const std::string & name)
  : file_(file), line_(line), column_(column),
  type_(type), name_(name)
{
}

const vds::vrt_method * vds::vrt_variable_declaration::init_value() const
{
  return this->init_value_.get();
}
