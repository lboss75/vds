#ifndef __VRT_TRY_CONTEXT_H_
#define __VRT_TRY_CONTEXT_H_

namespace vds {
  class vrt_method_context;

  class vrt_try_context
  {
  public:
  private:
    vrt_method_context * method_context_;
    int stack_size_;
    int statement_;
  };
}

#endif // __VRT_TRY_CONTEXT_H_
