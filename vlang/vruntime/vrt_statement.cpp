#include "stdafx.h"
#include "vrt_statement.h"
#include "vrt_context.h"
#include "vrt_method_context.h"
#include "vrt_variable.h"
#include "vrt_object.h"

vds::vrt_statement::vrt_statement(const vds::vrt_source_file* file, int line, int column)
  : file_(file), line_(line), column_(column)
{
}

vds::vrt_statement::~vrt_statement()
{
}

vds::vrt_pop_statement::vrt_pop_statement(
  const vds::vrt_source_file* file,
  int line, int column)
: vrt_statement(file, line, column)
{
}

bool vds::vrt_pop_statement::execute(
  vds::vrt_context& context) const
{
  context.pop();
  return true;
}

vds::vrt_assign_var_statement::vrt_assign_var_statement(const vrt_source_file * file, int line, int column, size_t variable_index)
  : vrt_statement(file, line, column), variable_index_(variable_index)
{
}

bool vds::vrt_assign_var_statement::execute(
  vrt_context & context) const
{
  auto value = context.pop();
  context.get_current_method()->get_variable(this->variable_index_)->set_value(value);
  return true;
}

vds::vrt_return_statement::vrt_return_statement(const vrt_source_file * file, int line, int column, const vrt_type * type)
  : vrt_statement(file, line, column), type_(type)
{
}

bool vds::vrt_return_statement::execute(
  vrt_context & context) const
{
  if (nullptr != this->type_){
    auto value = context.pop();
    context.method_return(value);
  }
  else {
    context.method_return();
  }
  
  return true;
}
