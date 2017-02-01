#ifndef __VRT_INJECTION_H_
#define __VRT_INJECTION_H_

#include "vrt_scope.h"

namespace vds {
  class vrt_context;
  class vrt_injection_lifetime;
  class vrt_object;
  class vrt_type;

  class vrt_injection : public vrt_scope
  {
  public:
    void register_transient(
      const vrt_type * service_interface,
      const vrt_type * implementation
    );
    
    void register_singleton(
      const vrt_type * service_interface,
      const vrt_type * implementation
    );

    void register_service(
      const vrt_injection_lifetime * lifetime,
      const vrt_type * service_interface,
      const vrt_type * implementation
      );

  private:
    struct service
    {
      const vrt_injection_lifetime * lifetime_;
      const vrt_type * service_interface_;
      const vrt_type * implementation_;
    };

    std::map<std::string, service> services_;
  };
}

#endif // !__VRT_INJECTION_H_
