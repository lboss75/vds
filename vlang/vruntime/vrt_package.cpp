#include "stdafx.h"
#include "vrt_package.h"
#include "vrt_statement.h"
#include "vrt_variable.h"
#include "vrt_variable_declaration.h"
#include "vrt_json.h"

vds::vrt_member_info::vrt_member_info(
  const vds::vrt_type* declaring_type,
  const std::string& name)
: declaring_type_(declaring_type), name_(name)
{
}

vds::vrt_member_info::~vrt_member_info()
{
}


vds::vrt_callable::vrt_callable(
  const vrt_type * declaring_type,
  const std::string & name)
: vds::vrt_member_info(declaring_type, name)
{
}

size_t vds::vrt_callable::get_parameter_index(const std::string & name) const
{
  size_t index = 0;
  for (const std::unique_ptr<vrt_parameter> & p : this->parameters_) {
    if (p->name() == name) {
      return index;
    }

    ++index;
  }

  return (size_t)-1;
}


vds::vrt_type::vrt_type(const vrt_package * package, const std::string& name)
: package_(package), name_(name)
{
}

vds::vrt_type::~vrt_type()
{
}

std::string vds::vrt_type::full_name() const
{
  return this->name_ + "," + this->package_->name();
}

vds::vrt_class::vrt_class(
  const vrt_package * package,
  const std::string& name,
  const vrt_source_file * file,
  int line,
  int column
)
: vrt_type(package, name), file_(file), line_(line), column_(column)
{
}

vds::vrt_source_file::vrt_source_file(const std::string& file_path)
: file_path_(file_path)
{
}

vds::vrt_method::vrt_method(
  const vrt_source_file * file,
  int line, int column,
  const vrt_type * declaring_type,
  const std::string& name)
: vrt_callable(declaring_type, name),
file_(file), line_(line), column_(column)
{
}

const vds::vrt_class * vds::vrt_package::get_class(const std::string & name, bool throwError /*= true*/) const
{
  for(const std::unique_ptr<vrt_class> & cls : this->classes_) {
    if (cls->name() == name) {
      return cls.get();
    }
  }
  
  if (throwError) {
    throw new std::runtime_error("Class " + name + " not found");
  }

  return nullptr;
}

vds::vrt_package::vrt_package(const std::string& name)
: name_(name)
{
}

vds::vrt_property::vrt_property(
  const vrt_type * declaring_type,
  const std::string & name)
  : vrt_member_info(declaring_type, name)
{
}

vds::vrt_parameter::vrt_parameter(
  const vrt_source_file * file,
  int line,
  int column,
  const vrt_type * type,
  const std::string & name
)
  : vrt_variable_declaration(file, line, column, type, name)
{
}

vds::vrt_external_method::vrt_external_method(
  const vrt_source_file * file,
  int line,
  int column,
  const vrt_type * declaring_type,
  const std::string & name)
  : vrt_callable(declaring_type, name)
{
}

bool vds::vrt_external_method::invoke(
  vrt_context & context,
  const std::shared_ptr<vrt_object>& pthis,
  const std::map<std::string, std::shared_ptr<vrt_object>>& arguments) const
{
  return this->impl_(
    context,
    pthis,
    arguments);
}

vds::vrt_constructor::vrt_constructor(
  const vds::vrt_source_file* file,
  int line,
  int column,
  const vrt_type * declaring_type
)
: vrt_method(file, line, column, declaring_type, std::string())
{

}


const std::string & vds::vrt_property::verb_get()
{
  static std::string result("get");
  return result;
}