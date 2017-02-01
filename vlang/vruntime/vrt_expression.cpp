#include "stdafx.h"
#include "vrt_expression.h"
#include "vrt_context.h"
#include "vrt_variable.h"
#include "vrt_object.h"
#include "vrt_package.h"
#include "vrt_method_context.h"

vds::vrt_expression::vrt_expression(const vrt_source_file * file, int line, int column)
  : vrt_statement(file, line, column)
{
}

vds::vrt_new_object_expression::vrt_new_object_expression(
  const vrt_source_file * file,
  int line,
  int column,
  const vrt_constructor * constructor,
  const std::list<std::string> & arguments)
  : vrt_expression(file, line, column),
  constructor_(constructor), arguments_(arguments)
{
}

bool vds::vrt_new_object_expression::execute(
  vrt_context & context
) const
{
  std::map<std::string, std::shared_ptr<vrt_object>> arguments;
  for(auto arg_name : this->arguments_) {
    arguments[arg_name] = context.pop();
  }

  std::shared_ptr<vrt_object> pthis(new vrt_object(this->constructor_->declaring_type()));
  return context.invoke(
    pthis,
    this->constructor_,
    arguments);
}


vds::vrt_type_reference::vrt_type_reference(
  const vds::vrt_source_file* file,
  int line,
  int column,
  const vds::vrt_type * type)
: vrt_expression(file, line, column), type_(type)
{
}


vds::vrt_get_property::vrt_get_property(
  const vds::vrt_source_file* file,
  int line, int column,
  const vrt_property * property)
: vrt_expression(file, line, column),
property_(property)
{

}

bool vds::vrt_get_property::execute(
  vds::vrt_context& context) const
{
  auto value = context.pop();
  return context.invoke(
    value,
    this->property_->get_method(vrt_property::verb_get()));
}

vds::vrt_localvariable_reference::vrt_localvariable_reference(const vds::vrt_source_file* file, int line, int column, size_t index)
: vrt_expression(file, line, column), index_(index)
{
}

bool vds::vrt_localvariable_reference::execute(
  vds::vrt_context& context) const
{
  context.push(context.get_current_method()->get_variable(this->index_)->get_value());
  return true;
}

vds::vrt_string_const::vrt_string_const(const vds::vrt_source_file* file, int line, int column, const std::string& value)
: vrt_expression(file, line, column), value_(value)
{
}

bool vds::vrt_string_const::execute(
  vds::vrt_context & context) const
{
  context.push_string_const(
    this->value_);
  return true;
}


vds::vrt_number_const::vrt_number_const(const vds::vrt_source_file* file, int line, int column, const std::string& value)
: vrt_expression(file, line, column), value_(value)
{

}

bool vds::vrt_number_const::execute(
  vrt_context & context) const
{
  context.push_number_const(
    this->value_);
  return true;
}

vds::vrt_method_invoke::vrt_method_invoke(const vrt_source_file * file, int line, int column, const vrt_callable * method, const std::list<std::string>& arguments)
  : vrt_expression(file, line, column),
  method_(method), arguments_(arguments)
{
}

bool vds::vrt_method_invoke::execute(
  vrt_context & context) const
{
  std::map<std::string, std::shared_ptr<vrt_object>> arguments;
  for (auto arg_name : this->arguments_) {
    arguments[arg_name] = context.pop();
  }

  auto pthis = context.pop();
  return context.invoke(
    pthis,
    this->method_,
    arguments);
}
