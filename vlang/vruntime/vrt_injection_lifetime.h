#ifndef __VRT_INJECTION_LIFETIME_H_
#define __VRT_INJECTION_LIFETIME_H_

#include "vrt_scope.h"

namespace vds {
  class vrt_context;
  class vrt_object;
  class vrt_type;

  class vrt_injection_lifetime
  {
  public:
    static const vrt_injection_lifetime * singleton();
    static const vrt_injection_lifetime * transient();

  private:
  };
}

#endif // !__VRT_INJECTION_LIFETIME_H_
