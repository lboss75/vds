#ifndef __VRT_SCOPE_H_
#define __VRT_SCOPE_H_

namespace vds {
  class vrt_injection;
  class vrt_object;
  class vrt_type;
  class vruntime_machine;
  class vrt_variable;
  class vrt_context;

  class vrt_scope
  {
  public:
    vrt_scope();
    virtual ~vrt_scope();
  };  
  
}

#endif // __VRT_SCOPE_H_
