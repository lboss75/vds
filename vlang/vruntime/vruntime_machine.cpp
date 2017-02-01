#include "stdafx.h"
#include "vruntime_machine.h"
#include "vrt_package.h"
#include "vrt_context.h"
#include "vrt_try_context.h"
#include "vrt_method_context.h"
#include "vrt_statement.h"
#include "vrt_variable.h"
#include "vrt_json.h"
#include "vrt_object.h"
#include "vrt_scope.h"
#include "vrt_injection.h"
#include "vrt_variable_declaration.h"

vds::vruntime_machine::vruntime_machine()
: registrations_(new vrt_injection())
{
}

void vds::vruntime_machine::run(
  const std::function<void(const std::string & service_name, const std::map<std::string, const vrt_json_value *> & service_properties)> & service_starter)
{
  //register injections
  for (std::map<std::string, std::unique_ptr<vrt_package>>::const_iterator p = this->packages_.begin(); p != this->packages_.end(); ++p) {
    auto package_properties = p->second->properties();
    package_properties->visit([this, service_starter, p](const vrt_json_property & prop) {
      if (prop.name == "injection") {
        auto injections = dynamic_cast<const vrt_json_array *>(prop.value.get());
        if (nullptr != injections) {
          for(size_t i = 0; i < injections->size(); ++i) {
            std::string lifetime("transient");
            std::string implementation;
            std::string interface;
            
            static_cast<const vrt_json_object *>(injections->get(i))->visit(
              [&lifetime, &implementation, &interface](const vrt_json_property & prop) {
                auto val = dynamic_cast<const vrt_json_primitive *>(prop.value.get());
                if(nullptr != val && "lifetime" == prop.name) {
                  lifetime = val->value();
                }
                else if(nullptr != val && "interface" == prop.name) {
                  interface = val->value();
                }
                else if(nullptr != val && "implementation" == prop.name) {
                  implementation = val->value();
                }
            });
            
            auto rt_interface = this->get_type(interface);
            auto rt_implementation = this->get_type(implementation);
            
            if("transient" == lifetime){
              this->registrations_->register_transient(
                rt_interface,
                rt_implementation
              );
            }
            else if("singleton" == lifetime){
              this->registrations_->register_singleton(
                rt_interface,
                rt_implementation
              );
            }
            else{
              throw new std::runtime_error("Invalid lifetime " + lifetime);
            }
          }
        }
      }
    });
  }
  
  //register services
  for (std::map<std::string, std::unique_ptr<vrt_package>>::const_iterator p = this->packages_.begin(); p != this->packages_.end(); ++p) {
    auto package_properties = p->second->properties();
    package_properties->visit([this, service_starter](const vrt_json_property & prop) {
      if (prop.name == "services") {
        auto services = dynamic_cast<const vrt_json_object *>(prop.value.get());
        if (nullptr != services) {
          services->visit([this, service_starter](const vrt_json_property & service_info) {
            this->start_service(service_starter, service_info);
          });
        }
      }
    });
  }
}

vds::vrt_package* vds::vruntime_machine::create_package(const std::string & name)
{
  auto p = this->packages_.find(name);
  if(this->packages_.end() != p){
    throw new std::logic_error("Package " + name + " already defined");
  }
  
  auto result = new vrt_package(name);
  this->packages_[name] = std::unique_ptr<vrt_package>(result);
  
  return result;
}

void vds::vruntime_machine::start_service(const std::function<void(const std::string & service_name, const std::map<std::string, const vrt_json_value *> & service_properties)> & service_starter, const vrt_json_property & service_info)
{
  auto properties = dynamic_cast<const vrt_json_object *>(service_info.value.get());
  if (nullptr != properties) {
    std::map<std::string, const vrt_json_value *> service_properties;
    properties->visit([&service_properties](const vrt_json_property & service_property) {
      service_properties[service_property.name] = service_property.value.get();
    });

    auto ptype = service_properties.find("$type");
    if (service_properties.end() == ptype) {
      throw new std::runtime_error("Invalid service type");
    }

    auto tname = dynamic_cast<const vrt_json_primitive *>(ptype->second);
    if (nullptr == tname) {
      throw new std::runtime_error("Invalid service type");
    }

    service_starter(tname->value(), service_properties);
  }
}

const vds::vrt_package* vds::vruntime_machine::get_core_package() const
{
  return this->get_package("core");
}

const vds::vrt_package* vds::vruntime_machine::get_package(const std::string & name) const
{
  auto p = this->packages_.find(name);
  if (this->packages_.end() == p) {
    throw new std::runtime_error("Invalid package name " + name);
  }

  return p->second.get();
}

const vds::vrt_type* vds::vruntime_machine::get_type(const std::string& type_name, bool throw_error) const
{
  auto pos = type_name.find(',');
  if (std::string::npos == pos) {
    throw new std::runtime_error("Invalid type " + type_name);
  }

  auto package_name = type_name.substr(pos + 1);
  auto package = this->get_package(package_name);

  auto class_name = type_name.substr(0, pos);
  auto cls = package->get_class(class_name);
  
  return cls;
}

const vds::vrt_constructor * vds::vruntime_machine::resolve_constructor(
  const vds::vrt_type* type,
  const std::map< std::string, const vds::vrt_type *> & argument_types,
  bool throw_error /*= true*/) const
{
  auto cls = dynamic_cast<const vrt_class *>(type);
  if (nullptr != cls) {
    for (const std::unique_ptr<vrt_constructor> & constructor : cls->constructors()) {
      if (match_parameters(constructor->parameters(), argument_types)) {
        return constructor.get();
      }
    }
  }

  if (throw_error) {
    throw new std::runtime_error("Constructor for " + type->full_name() + " is not found");
  }

  return nullptr;
}

const vds::vrt_callable * vds::vruntime_machine::resolve_method(
  const vrt_type * object_type,
  const std::string & name,
  const std::map<std::string, const vrt_type*> & argument_types) const
{
  auto cls = dynamic_cast<const vrt_class *>(object_type);
  if (nullptr != cls) {
    for (const std::unique_ptr<vrt_callable> & method : cls->methods()) {
      if (match_parameters(method->parameters(), argument_types)) {
        return method.get();
      }
    }
  }

  return nullptr;
}

bool vds::vruntime_machine::match_parameters(
  const std::vector<std::unique_ptr<vds::vrt_parameter>> & method_parameters,
  const std::map< std::string, const vds::vrt_type *> & argument_types) const
{
  std::map<std::string, const vrt_parameter *> parameters;
  for (const std::unique_ptr<vrt_parameter> & parameter : method_parameters) {
    parameters[parameter->name()] = parameter.get();
    auto argument = argument_types.find(parameter->name());
    if (argument_types.end() == argument) {
      if (nullptr == parameter->init_value()) {
        return false;
      }
    }
    else {
      if (argument->second != parameter->type()) {
        return false;
      }
    }
  }

  for (auto argument : argument_types) {
    auto p = parameters.find(argument.first);
    if (parameters.end() == p) {
      return false;
    }
    if (p->second->type() != argument.second) {
      return false;
    }
  }

  return true;
}

void vds::vruntime_machine::create_object(
  const std::function<void(const std::shared_ptr<vrt_object> &)> & done,
  const error_handler_t & on_error,
  const vrt_constructor * constructor,
  const std::map<std::string, std::shared_ptr<vrt_object>> & arguments
)
{
  std::shared_ptr<vrt_object> pthis(new vrt_object(constructor->declaring_type()));

  std::shared_ptr<vrt_context> context(new vrt_context(this,
    [done, pthis](vrt_context & context) {
      done(pthis);
    },
    on_error));

  if(context->invoke(
    pthis,
    constructor,
    arguments)){
    context->execute_continue();
  }
}

void vds::vruntime_machine::invoke(
  const std::function<void(const std::shared_ptr<vrt_object> &)> & done,
  const error_handler_t & on_error,
  const std::shared_ptr<vrt_object> & pthis,
  const vrt_callable * method,
  const std::map<std::string, std::shared_ptr<vrt_object>> & arguments
)
{
  std::shared_ptr<vrt_context> context(new vrt_context(this,
    [done, pthis, method](vrt_context & context) {
      std::shared_ptr<vrt_object> value;
      if(nullptr != method->get_result_type()){
        value = context.pop();
      }
    
      done(value);
  },
    on_error));

  if(context->invoke(
    pthis,
    method,
    arguments)) {
    context->execute_continue();
  }
}
