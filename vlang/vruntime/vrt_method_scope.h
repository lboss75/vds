#ifndef __VRT_METHOD_SCOPE_H_
#define __VRT_METHOD_SCOPE_H_

namespace vds {
  class vrt_method;

  class vrt_method_scope
  {
  public:
    void invoke(const vrt_method * method);

  private:
    void execute();
  };
}

#endif//__VMETHOD_SCOPE_H_
