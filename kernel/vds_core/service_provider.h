#ifndef __VDS_CORE_SERVICE_PROVIDER_H_
#define __VDS_CORE_SERVICE_PROVIDER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "types.h"

namespace vds {
  class shutdown_event;
  class service_registrator;
  class _service_provider;
  class _service_registrator;

  class service_provider
  {
  public:

    static service_provider empty();

    template <typename service_type>
    service_type * get(bool throw_error = true) const
    {
      auto result = (service_type *)this->get(types::get_type_id<service_type>());
      if (throw_error && nullptr == result) {
        throw std::runtime_error(std::string("Service ") + typeid(service_type).name() + " not found");
      }

      return result;
    }

    service_provider create_scope(const std::string & name) const;

    size_t id() const;
    const std::string & name() const;
    const std::string & full_name() const;

    shutdown_event & get_shutdown_event() const;

    class property_holder
    {
    public:
      virtual ~property_holder();
    };

    enum class property_scope
    {
      root_scope,
      local_scope,
      any_scope
    };

    template <typename property_type>
    const property_type * get_property(property_scope scope) const
    {
      return static_cast<const property_type *>(this->get_property(scope, types::get_type_id<property_type>()));
    }

    template <typename property_type>
    void set_property(property_scope scope, property_type * value)
    {
      this->set_property(scope, types::get_type_id<property_type>(), value);
    }

    bool operator ! () const
    {
      return !this->impl_;
    }
    
    void unhandled_exception(std::exception_ptr ex = std::current_exception());
  private:
    friend class _service_provider;
    friend class _service_registrator;

    service_provider(std::shared_ptr<_service_provider> && impl);

    std::shared_ptr<_service_provider> impl_;

    void * get(size_t type_id) const;
    const property_holder * get_property(property_scope scope, size_t type_id) const;
    void set_property(property_scope scope, size_t type_id, property_holder * value);
  };

  class iservice_factory
  {
  public:
    virtual void register_services(service_registrator &) = 0;
    virtual void start(const service_provider &) = 0;
    virtual void stop(const service_provider &) = 0;
  };
  

  class service_registrator
  {
  public:
    service_registrator();

    template <typename service_type>
    void add_service(service_type * service)
    {
      this->add_service(types::get_type_id<service_type>(), service);
    }

    void add(iservice_factory & factory);

    void shutdown(service_provider & sp);

    service_provider build(const std::string & name);
    void start(const service_provider & sp);

  private:
    friend class _service_registrator;
    std::shared_ptr<_service_registrator> impl_;

    void add_service(size_t type_id, void * service);
  };

  class unhandled_exception_handler : public service_provider::property_holder
  {
  public:
    unhandled_exception_handler(const std::function<void(const service_provider & sp, std::exception_ptr ex)> & handler)
      : on_error(handler)
    {
    }

    std::function<void(const service_provider & sp, std::exception_ptr ex)> on_error;
  };
}


#endif // ! __VDS_CORE_SERVICE_PROVIDER_H_


