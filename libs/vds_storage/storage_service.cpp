/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "storage_service.h"

void vds::storage_service::register_services(service_registrator & registrator)
{
  registrator.add_factory<istorage>(
    [this](const service_provider &, bool & ) {
      return istorage(this);
  });
}

void vds::storage_service::start(const service_provider &)
{
}

void vds::storage_service::stop(const service_provider &)
{
}
