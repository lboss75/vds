#include "stdafx.h"
#include "vtype_resolver.h"
#include "vfile_syntax.h"
#include "name_resolver.h"
#include "compile_error.h"

vds::vtype_resolver::vtype_resolver(
  vds::vruntime_machine* machine,
  const vds::vrt_source_file* file,
  vds::name_resolver& resolver)
: machine_(machine), file_(file),
resolver_(resolver)
{

}

void vds::vtype_resolver::resolve_types(vrt_callable * target, const vmethod * source)
{
  if(nullptr != source->result_type()){
    target->result_type_ = this->resolve_type(source->result_type());
  }
  for(const std::unique_ptr<vmethod_parameter> & p : source->parameters()){
    target->parameters_.push_back(
      std::unique_ptr<vrt_parameter>(
        new vrt_parameter(
          this->file_,
          p->line(),
          p->column(),
          this->resolve_type(p->type()),
          p->name()
    )));
  }
}

const vds::vrt_type * vds::vtype_resolver::resolve_type(const std::string& name, bool throw_error)
{
  for(const std::string & ns : this->namespaces_) {
    auto result = this->resolver_.resolve_type(ns + "." + name);
    if(result){
      return result;
    }
  }
  
  auto result = this->resolver_.resolve_type(name);
  if(nullptr == result && throw_error){
    throw new std::runtime_error(
      "Type " + name + " not found");
  }
  
  return result;
}

const vds::vrt_type * vds::vtype_resolver::resolve_type(const vds::vtype * t, bool throw_error)
{
  auto result = this->resolve_type(t->name(), throw_error);
  if(!result){
    throw new compile_error(
      t->owner().file_path(),
      t->line(),
      t->column(),
      "Type " + t->name() + " not resolved");
  }
  
  return result;
}

const vds::vrt_type* vds::vtype_resolver::resolve_name(vds::vmethod_compiler& compiler, const std::string& name)
{
  return this->resolver_.resolve_name(compiler, name);

}
