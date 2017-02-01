#include "stdafx.h"
#include "vrt_package.h"
#include "vrt_external_method_resolver.h"
#include "vrt_external_method_resolve_exception.h"

vds::vrt_external_method_resolver::method_body_t vds::vrt_external_method_resolver::resolve(const vrt_external_method * method) const
{
  auto fullname = method->declaring_type()->name() + "." + method->name();
  auto p = this->methods_.find(fullname);
  if(this->methods_.end() == p){
    throw new vrt_external_method_resolve_exception(
      "Unable to resolve method " + fullname);
  }
  
  return p->second;
}

void vds::vrt_external_method_resolver::add(const std::string& name, const method_body_t method)
{
  this->methods_[name] = method;
}

