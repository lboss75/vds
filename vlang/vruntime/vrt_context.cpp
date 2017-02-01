#include "stdafx.h"
#include "vrt_context.h"
#include "vrt_package.h"
#include "vrt_object.h"
#include "vrt_method_context.h"
#include "vrt_statement.h"
#include "vrt_try_context.h"
#include "vruntime_machine.h"
#include "vrt_variable.h"
#include "vrt_scope.h"

vds::vrt_context::vrt_context(
  vds::vruntime_machine* machine,
  const std::function<void(vrt_context &)> & done,
  const error_handler_t & on_error)
: machine_(machine), final_done_(done), final_error_(on_error),
  done_handler_(std::bind(&vds::vrt_context::execute_continue, this)),
  error_handler_(std::bind(&vds::vrt_context::throw_error, this, std::placeholders::_1))
{
}

void vds::vrt_context::push(
  const std::shared_ptr<vds::vrt_object>& value)
{
  this->methods_.top()->push(value);
}

std::shared_ptr<vds::vrt_object> vds::vrt_context::pop()
{
  return this->methods_.top()->pop();
}

void vds::vrt_context::method_return(
  const std::shared_ptr<vrt_object> & value)
{
  this->methods_.pop();
  this->push(value);
}

void vds::vrt_context::method_return()
{
  this->methods_.pop();
}

bool vds::vrt_context::invoke(
  const std::shared_ptr< vds::vrt_object >& pthis,
  const vds::vrt_callable * method,
  const std::map< std::string, std::shared_ptr< vds::vrt_object > >& arguments)
{
  auto external_method = dynamic_cast<const vrt_external_method *>(method);
  if (nullptr != external_method) {
    return external_method->invoke(
      *this, pthis, arguments);
  }
  
  auto vmethod = static_cast<const vrt_method *>(method);
  
  auto context = new vrt_method_context(*this, pthis, vmethod, arguments);
  this->methods_.push(std::unique_ptr<vrt_method_context>(context));
  return true;
}

vds::vrt_method_context * vds::vrt_context::get_current_method() const
{
  return this->methods_.top().get();
}

bool vds::vrt_context::resolve_dependency(const vrt_type * interface_type)
{

  return this->throw_error(new std::runtime_error("Unable to resolve dependency "
  + interface_type->full_name()));
}

void vds::vrt_context::execute_continue()
{
  while (this->execute_step()) {
  };
}

bool vds::vrt_context::throw_error(std::exception * ex)
{
  throw ex;
}

bool vds::vrt_context::execute_step()
{
  if (this->methods_.empty()) {
    this->final_done_(*this);
    return false;
  }

  return this->methods_.top()->execute_step(*this);
}

void vds::vrt_context::push_string_const(
  const std::string& value)
{
  this->push(
    std::shared_ptr<vds::vrt_object>(
      new vds::vrt_string_object(
        this->machine_->get_core_package()->get_class("string"),
        value)));
}

void vds::vrt_context::push_number_const(
  const std::string & value)
{
  this->push(
    std::shared_ptr<vds::vrt_object>(
      new vds::vrt_string_object(
        this->machine_->get_core_package()->get_class("number"),
        value)));
}
