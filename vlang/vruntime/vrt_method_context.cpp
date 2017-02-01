#include "stdafx.h"
#include "vrt_method_context.h"
#include "vrt_package.h"
#include "vrt_expression.h"
#include "vrt_variable.h"
#include "vrt_scope.h"

vds::vrt_method_context::vrt_method_context(
  vrt_context & context,
  const std::shared_ptr< vrt_object >& pthis,
  const vds::vrt_method * method,
  const std::map<std::string, std::shared_ptr<vrt_object> >& arguments)
: context_(context), this_(pthis), method_(method),
next_statement_(0), variables_(method->parameters().size() + method->variables_.size())
{
  for(auto argument : arguments){
    auto index = method->get_parameter_index(argument.first);
    this->variables_[index] = std::shared_ptr<vrt_variable>(
      new vrt_variable(
        method->parameters()[index].get(),
        argument.second));
  }
}


void vds::vrt_method_context::push(const std::shared_ptr< vrt_object >& value)
{
  this->stack_.push(value);
}

bool vds::vrt_method_context::execute_step(vrt_context & context)
{
  return this->method_->body().at(this->next_statement_++)->execute(context);
}

std::shared_ptr<vds::vrt_object> vds::vrt_method_context::pop()
{
  std::shared_ptr<vds::vrt_object> result = this->stack_.top();
  this->stack_.pop();
  return result;
}

