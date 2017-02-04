/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "service_provider.h"

vds::iservice::~iservice()
{
}

vds::iservice_provider_impl::iservice_provider_impl()
{
}

vds::iservice_provider_impl::~iservice_provider_impl()
{
    for (auto handler : this->done_handlers_) {
        handler();
    }

    for (auto p : this->scopped_objects_) {
        delete p.second;
    }
}

void vds::iservice_provider_impl::on_complete(const std::function<void(void)>& done)
{
    this->done_handlers_.push_back(done);
}

vds::service_provider vds::iservice_provider_impl::create_scope()
{
    return service_provider(
      new scopped_service_provider(
        this->shared_from_this()));
}

//////////////////////////////////////////////////////
vds::scopped_service_provider::scopped_service_provider(
    const std::shared_ptr<vds::iservice_provider_impl> & parent)
    : parent_(parent)
{
}

const vds::shutdown_event & vds::scopped_service_provider::get_shutdown_event() const
{
    return this->parent_->get_shutdown_event();
}

vds::iservice_provider_impl::iservice_factory * vds::scopped_service_provider::get_factory(size_t type)
{
    return this->parent_->get_factory(type);
}

//////////////////////////////////////////////////////
vds::service_registrator::service_registrator()
    : impl_(new service_registrator_impl())
{
}

vds::service_provider vds::service_registrator::build() const
{
    return this->impl_->build();
}

void vds::service_registrator::add(iservice & service)
{
    service.register_services(*this);
    this->impl_->add(service);
}

void vds::service_registrator::shutdown()
{
    this->impl_->shutdown();
}
//////////////////////////////////////////////////////
vds::service_provider::service_provider(iservice_provider_impl * impl)
    : impl_(impl)
{
}

vds::service_provider::service_provider(const std::shared_ptr<iservice_provider_impl>& impl)
    : impl_(impl)
{
}

vds::service_provider vds::service_provider::create_scope() const
{
    return this->impl_->create_scope();
}

void vds::service_provider::on_complete(const std::function<void(void)>& done) const
{
    this->impl_->on_complete(done);
}

const vds::shutdown_event & vds::service_provider::get_shutdown_event() const
{
    return this->impl_->get_shutdown_event();
}


//////////////////////////////////////////////////////
vds::service_registrator_impl::service_registrator_impl()
{
}

vds::service_registrator_impl::~service_registrator_impl()
{
}

void vds::service_registrator_impl::add(iservice & service)
{
    this->services_.push_back(&service);
}

void vds::service_registrator_impl::shutdown()
{
    service_provider result(this->shared_from_this());

    this->shutdown_event_.set();
    while (!this->services_.empty()) {
        this->services_.front()->stop(result);
        this->services_.pop_front();
    }
}

vds::service_provider vds::service_registrator_impl::build()
{
    service_provider result = this->create_scope();

    for (auto service : this->services_) {
        service->start(result);
    }

    return result;
}

vds::iservice_provider_impl::iservice_factory::~iservice_factory()
{
}

vds::iservice_provider_impl::iobject_holder::~iobject_holder()
{
}
