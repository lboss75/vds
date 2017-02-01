#include "stdafx.h"
#include "vrt_external_method_resolve_exception.h"

vds::vrt_external_method_resolve_exception::vrt_external_method_resolve_exception(const std::string& message)
: std::runtime_error(message)
{
}
