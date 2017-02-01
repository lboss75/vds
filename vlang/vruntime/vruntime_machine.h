#ifndef __VRUNTIME_MACHINE_H_
#define __VRUNTIME_MACHINE_H_

#include "func_utils.h"

namespace vds {
  class vrt_callable;
  class vrt_class;
  class vrt_context;
  class vrt_constructor;
  class vrt_injection;
  class vrt_json_value;
  struct vrt_json_property;
  class vrt_object;
  class vrt_package;
  class vrt_parameter;
  class vrt_type;
  
  class vruntime_machine
  {
  public:
    vruntime_machine();
    
    void run(
      const std::function<void (const std::string & service_name, const std::map<std::string, const vrt_json_value *> & service_properties)> & service_starter);
    
    const vrt_package * get_core_package() const;
    const vrt_package * get_package(const std::string & name) const;
    
    const vrt_type * get_type(
      const std::string & type_name,
      bool throw_error = true) const;

    const vrt_constructor * resolve_constructor(
      const vds::vrt_type* type,
      const std::map< std::string, const vds::vrt_type *> & argument_types,
      bool throw_error = true) const;

    const vrt_callable * resolve_method(
      const vrt_type * object_type,
      const std::string & name,
      const std::map<std::string, const vrt_type*> & argument_types) const;

    bool match_parameters(
      const std::vector<std::unique_ptr<vrt_parameter>> & method_parameters,
      const std::map<std::string, const vrt_type *> & argument_types) const;

    void create_object(
      const std::function<void(const std::shared_ptr<vrt_object> &)> & done,
      const error_handler_t & on_error,
      const vrt_constructor * constructor,
      const std::map<std::string, std::shared_ptr<vrt_object>> & arguments
    );
    
    void invoke(
      const std::function<void(const std::shared_ptr<vrt_object> &)> & done,
      const error_handler_t & on_error,
      const std::shared_ptr<vrt_object> & pthis,
      const vrt_callable * method,
      const std::map<std::string, std::shared_ptr<vrt_object>> & arguments
    );

  private:
    friend class vpackage_compiler;
    friend class vmethod_compiler;
    
    std::map<std::string, std::unique_ptr<vrt_package>> packages_;
    std::unique_ptr<vrt_injection> registrations_;
    
    vrt_package * create_package(const std::string & name);

    void start_service(
      const std::function<void(
        const std::string & /*service_name*/,
        const std::map<std::string, const vrt_json_value *> & /*service_properties*/
      )> & service_starter,
      const vrt_json_property & service_info);
  };
}

#endif//__VRUNTIME_MACHINE_H_
