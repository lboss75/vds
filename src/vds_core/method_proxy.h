#ifndef __VDS_CORE_METHOD_PROXY_H_
#define __VDS_CORE_METHOD_PROXY_H_

namespace vds {
  template <
    typename method_type,
    typename method_signature>
  class method_proxy;

  template <
    typename method_type,
    typename... arg_types>
  class method_proxy<method_type, void(arg_types...)>
  {
  public:
    method_proxy(method_type & method)
      : method_(method)
    {
    }

    void operator()(arg_types... args)
    {
      this->method_.check_alive();
      this->method_(args...);
    }

  private:
    method_type & method_;
  };
}

#endif // __VDS_CORE_METHOD_PROXY_H_
