#include "stdafx.h"
#include "vrt_injection.h"
#include "vrt_package.h"
#include "vrt_context.h"
#include "vrt_injection_lifetime.h"

void vds::vrt_injection::register_transient(const vrt_type * service_interface, const vrt_type * implementation)
{
  this->register_service(vrt_injection_lifetime::transient(), service_interface, implementation);
}

void vds::vrt_injection::register_singleton(const vrt_type * service_interface, const vrt_type * implementation)
{
  this->register_service(vrt_injection_lifetime::singleton(), service_interface, implementation);
}

void vds::vrt_injection::register_service(
  const vrt_injection_lifetime * lifetime,
  const vrt_type * service_interface,
  const vrt_type * implementation)
{
  this->services_[service_interface->full_name()] = service{
    lifetime, service_interface, implementation
  };
}

