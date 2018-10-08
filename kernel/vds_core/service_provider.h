#ifndef __VDS_CORE_SERVICE_PROVIDER_H_
#define __VDS_CORE_SERVICE_PROVIDER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>
#include <unordered_map>
#include <list>
#include "types.h"
#include "async_task.h"
#include "shutdown_event.h"
#include "foldername.h"

namespace vds {
  class shutdown_event;
  class service_registrator;
  class foldername;

  class iservice_factory
  {
  public:
    virtual void register_services(service_registrator &) = 0;
    virtual void start(const service_provider *) = 0;
    virtual void stop() = 0;
    virtual std::future<void> prepare_to_stop() {
      co_return;
    }
  };

  class service_provider
  {
  public:
    service_provider(const service_provider *) = delete;
    service_provider(service_provider && original) = delete;

    template <typename service_type>
    service_type * get(bool throw_error = true) const
    {
      auto result = (service_type *)this->get(types::get_type_id<service_type>());
      if (throw_error && nullptr == result) {
        throw std::runtime_error(std::string("Service ") + typeid(service_type).name() + " not found");
      }

      return result;
    }

    const shutdown_event& get_shutdown_event() const;
    const foldername & current_user() const;
    const foldername & local_machine() const;

  protected:
    service_provider() {}

    private:

    void* get(size_t type_id) const;
  };

  class service_registrator : private service_provider
  {
  public:
    template <typename service_type>
    void add_service(service_type * service)
    {
      this->add_service(types::get_type_id<service_type>(), service);
    }

    void add(iservice_factory & factory)
    {
      this->factories_.push_back(&factory);
    }

    void shutdown() {
      this->shutdown_event_.set();
      std::this_thread::sleep_for(std::chrono::seconds(5));

      for (auto& p : this->factories_) {
        p->prepare_to_stop().get();
      }

      while (!this->factories_.empty()) {
        this->factories_.back()->stop();
        this->factories_.pop_back();
      }
    }

    service_provider * build() {
      for (auto factory : this->factories_) {
        factory->register_services(*this);
      }

      return this;
    }

    void start() {
      for (auto factory : this->factories_) {
        factory->start(this);
      }
    }

    void current_user(const foldername & value) {
      this->current_user_ = value;
    }

    void local_machine(const foldername & value) {
      this->local_machine_ = value;
    }

  private:
    friend class service_provider;

    shutdown_event shutdown_event_;
    std::unordered_map<size_t, void *> services_;
    std::list<iservice_factory *> factories_;
    foldername current_user_;
    foldername local_machine_;

    void add_service(size_t type_id, void* service) {
      if (this->services_.find(type_id) != this->services_.end()) {
        throw std::runtime_error("Invalid argument");
      }

      this->services_[type_id] = service;
    }
  };

  inline const vds::shutdown_event& vds::service_provider::get_shutdown_event() const {
    return static_cast<const service_registrator *>(this)->shutdown_event_;
  }

  inline const foldername& service_provider::current_user() const {
    return static_cast<const service_registrator *>(this)->current_user_;
  }

  inline const foldername& service_provider::local_machine() const {
    return static_cast<const service_registrator *>(this)->local_machine_;
  }

  inline void * service_provider::get(size_t type_id) const {
    auto p = static_cast<const service_registrator *>(this)->services_.find(type_id);
    if (static_cast<const service_registrator *>(this)->services_.end() == p) {
      return nullptr;
    }
    else {
      return p->second;
    }
  }

}


#endif // ! __VDS_CORE_SERVICE_PROVIDER_H_


