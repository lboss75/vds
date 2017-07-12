#ifndef __VDS_CORE_SERVICE_PROVIDER_P_H_
#define __VDS_CORE_SERVICE_PROVIDER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <unordered_map>
#include <list>
#include <atomic>

#include "service_provider.h"
#include "shutdown_event.h"

namespace vds {

  class _service_registrator : public std::enable_shared_from_this<_service_registrator>
  {
  public:

    void add_service(size_t type_id, void * service)
    {
      if (this->services_.find(type_id) != this->services_.end()) {
        throw std::runtime_error("Invalid argument");
      }

      this->services_[type_id] = service;
    }

    void * get(size_t type_id) const
    {
      auto p = this->services_.find(type_id);
      if (services_.end() == p) {
        return nullptr;
      }
      else {
        return p->second;
      }
    }

    void add(iservice_factory * factory)
    {
      this->factories_.push_back(factory);
    }

    void shutdown(service_provider & sp)
    {
      this->shutdown_event_.set();
      while (!this->factories_.empty()) {
        this->factories_.back()->stop(sp);
        this->factories_.pop_back();
      }
    }

    shutdown_event & get_shutdown_event() { return this->shutdown_event_; }

    service_provider build(
      service_registrator & owner,
      const char * name);

    void start(
      const service_provider & sp);

  private:
    shutdown_event shutdown_event_;
    std::unordered_map<size_t, void *> services_;
    std::list<iservice_factory *> factories_;
  };

  class _service_provider : public std::enable_shared_from_this<_service_provider>
  {
  public:
    _service_provider(
      const std::shared_ptr<_service_registrator> & service_registrator,
      const std::shared_ptr<_service_provider> & parent,
      const char * name)
    : id_(++s_last_id_),
      name_((name == nullptr) ? parent->name() : name),
      full_name_((name == nullptr) ? parent->full_name_ : ((parent ? (parent->full_name_ + "\n") : std::string()) + "[" + std::to_string(id_) + "]" + name)),
      parent_(parent),
      service_registrator_(service_registrator)
    {
    }

    service_provider create_scope(const service_provider * owner, const char * name) const
    {
      return service_provider(std::make_shared<_service_provider>(
        this->service_registrator_,
        owner->impl_,
        name));
    }

    size_t id() const { return this->id_; }
    const std::string & name() const { return this->name_; }
    const std::string & full_name() const { return this->full_name_; }

    shutdown_event & get_shutdown_event() const { return this->service_registrator_->get_shutdown_event(); }

    void * get(size_t type_id) const
    {
      return this->service_registrator_->get(type_id);
    }

    service_provider::property_holder * get_property(service_provider::property_scope scope, size_t type_id) const
    {
      auto pthis = this;
      while (pthis->parent_) {
        if (service_provider::property_scope::root_scope != scope) {
          auto p = pthis->properties_.find(type_id);
          if (pthis->properties_.end() != p) {
            return p->second.get();
          }
          else if (service_provider::property_scope::local_scope == scope) {
            return nullptr;
          }
        }

        pthis = pthis->parent_.get();
      }

      auto p = pthis->properties_.find(type_id);
      if (pthis->properties_.end() != p) {
        return p->second.get();
      }
      else {
        return nullptr;
      }
    }

    void set_property(service_provider::property_scope scope, size_t type_id, service_provider::property_holder * value)
    {
      auto pthis = this;

      if (service_provider::property_scope::root_scope == scope) {
        while (pthis->parent_) {
          pthis = pthis->parent_.get();
        }
      }

      pthis->properties_[type_id].reset(value);
    }

  private:
    static std::atomic_size_t s_last_id_;

    size_t id_;
    std::string name_;
    std::string full_name_;

    std::shared_ptr<_service_provider> parent_;
    std::shared_ptr<_service_registrator> service_registrator_;

    std::unordered_map<size_t, std::unique_ptr<service_provider::property_holder>> properties_;
  };
};

#endif // ! __VDS_CORE_SERVICE_PROVIDER_H_


