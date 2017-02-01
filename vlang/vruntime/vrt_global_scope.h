#ifndef __VRT_GLOBAL_SCOPE_H_
#define __VRT_GLOBAL_SCOPE_H_

namespace vds {
  class vrt_method;

  class vrt_global_scope
  {
  public:
    void invoke(const vrt_method * method);
  };
}

#endif//__VRT_GLOBAL_SCOPE_H_