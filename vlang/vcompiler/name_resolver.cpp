#include "stdafx.h"
#include "name_resolver.h"
#include "vrt_package.h"

void vds::name_resolver::add(const vds::vrt_package* package)
{
  this->packages_.push_back(package);
  
  for(const std::unique_ptr<vrt_class> & cls : package->classes()) {
    this->classes_[cls->name()] = cls.get();
  }
}

const vds::vrt_type * vds::name_resolver::resolve_type(const std::string& type_name)
{
  auto p = this->classes_.find(type_name);
  if(this->classes_.end() == p) {
    return nullptr;
  }
  
  return p->second;
}

const vds::vrt_type * vds::name_resolver::resolve_name(vmethod_compiler & compiler, const std::string& name)
{
  auto pname = this->names_.find(name);
  if(this->names_.end() == pname) {
    return nullptr;
  }
  
  return pname->second(compiler);
}


void vds::name_resolver::add_name(const std::string& name, const std::function< const vrt_type *(vmethod_compiler &) >& factory)
{
  this->names_[name] = factory;
}
