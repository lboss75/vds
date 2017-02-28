/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "storage_service.h"
#include "storage_log.h"

vds::istorage::istorage(storage_service * owner)
  : owner_(owner)
{
}

vds::storage_log & vds::istorage::get_storage_log() const
{
  return *this->owner_->storage_log_.get();
}

vds::storage_service::storage_service()
{

}

vds::storage_service::~storage_service()
{

}

void vds::storage_service::register_services(service_registrator & registrator)
{
  registrator.add_factory<istorage>(
    [this](const service_provider &, bool & ) {
      return istorage(this);
  });
}

void vds::storage_service::start(const service_provider & sp)
{
  this->storage_log_.reset(new storage_log(sp));
  this->storage_log_->start();
}

void vds::storage_service::stop(const service_provider &)
{
  this->storage_log_->stop();
}
