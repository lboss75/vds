#include "stdafx.h"
#include "vrt_injection_lifetime.h"

const vds::vrt_injection_lifetime * vds::vrt_injection_lifetime::singleton()
{
  static vrt_injection_lifetime result;
  return &result;
}

const vds::vrt_injection_lifetime * vds::vrt_injection_lifetime::transient()
{
  static vrt_injection_lifetime result;
  return &result;
}
