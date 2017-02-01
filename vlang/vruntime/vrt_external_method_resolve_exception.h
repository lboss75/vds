#ifndef __VRT_EXTERNAL_METHOD_RESOLVE_EXCEPTION_H_
#define __VRT_EXTERNAL_METHOD_RESOLVE_EXCEPTION_H_

#include <exception>

namespace vds {
  class vrt_external_method_resolve_exception : public std::runtime_error
  {
  public:
    vrt_external_method_resolve_exception(const std::string & message);
  };
}

#endif // __VRT_EXTERNAL_METHOD_RESOLVE_EXCEPTION_H_
