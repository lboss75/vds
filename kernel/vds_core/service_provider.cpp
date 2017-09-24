/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <iostream>
#include "service_provider.h"
#include "service_provider_p.h"
#include "logger.h"

vds::service_provider::service_provider(std::shared_ptr<_service_provider> && impl)
  : impl_(impl)
{
}

vds::service_provider vds::service_provider::empty()
{
  return service_provider(std::shared_ptr<_service_provider>());
}

vds::service_provider vds::service_provider::create_scope(const char * name) const
{
  return this->impl_->create_scope(this, name);
}

size_t vds::service_provider::id() const
{
  return this->impl_->id();
}

const std::string & vds::service_provider::name() const
{
  static std::string empty_string;
  return this->impl_ ? this->impl_->name() : empty_string;
}

const std::string & vds::service_provider::full_name() const
{
  return this->impl_->full_name();
}

vds::shutdown_event & vds::service_provider::get_shutdown_event() const
{
  return this->impl_->get_shutdown_event();
}

void * vds::service_provider::get(size_t type_id) const
{
  return this->impl_->get(type_id);
}

const vds::service_provider::property_holder * vds::service_provider::get_property(property_scope scope, size_t type_id) const
{
  return this->impl_->get_property(scope, type_id);
}

void vds::service_provider::set_property(property_scope scope, size_t type_id, property_holder * value) const
{
  this->impl_->set_property(scope, type_id, value);
}

void vds::service_provider::unhandled_exception(const std::shared_ptr<std::exception> & ex) const
{
  auto p = this->get_property<unhandled_exception_handler>(service_provider::property_scope::any_scope);
  if (nullptr != p) {
    p->on_error(*this, ex);
  }
  else {
    this->get<logger>()->error("sp", *this, "Unhandler error %s", ex->what());
    abort();
  }
}

//////////////////////////////////////
std::atomic_size_t vds::_service_provider::s_last_id_;

vds::service_provider vds::_service_registrator::build(
  service_registrator & owner,
  const char * name)
{
  auto sp = service_provider(
    std::make_shared<_service_provider>(
      owner.impl_, std::shared_ptr<_service_provider>(), name));
  for (auto factory : this->factories_) {
    factory->register_services(owner);
  }

  return sp;
}

void vds::_service_registrator::start(const service_provider & sp)
{
  for (auto factory : this->factories_) {
    factory->start(sp);
  }
}

//////////////////////////////////////

vds::service_provider::property_holder::~property_holder()
{
}

vds::service_registrator::service_registrator()
  : impl_(std::make_shared<_service_registrator>())
{
}

void vds::service_registrator::add(iservice_factory & factory)
{
  this->impl_->add(&factory);
}

void vds::service_registrator::shutdown(service_provider & sp)
{
  this->impl_->shutdown(sp);
}

vds::service_provider vds::service_registrator::build(const char * name)
{
  return this->impl_->build(*this, name);
}

void vds::service_registrator::start(const service_provider & sp)
{
  this->impl_->start(sp);
}

void vds::service_registrator::add_service(size_t type_id, void * service)
{
  this->impl_->add_service(type_id, service);
}
