#ifndef __VDS_CORE_METHOD_PROXY_H_
#define __VDS_CORE_METHOD_PROXY_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  template <
    typename method_type,
    typename method_signature>
  class method_proxy;

  template <
    typename method_type,
    typename result_type,
    typename... arg_types>
  class method_proxy<method_type, result_type(arg_types...)>
  {
  public:
    method_proxy(const method_proxy&) = delete;
    method_proxy(method_proxy&&) = delete;
    method_proxy & operator = (const method_proxy &) = delete;
    method_proxy & operator = (method_proxy &&) = delete;
    
    method_proxy(method_type & method)
      : method_(method)
    {
    }

    result_type operator()(arg_types... args)
    {
      this->method_.check_alive();
      return this->method_(std::forward<arg_types>(args)...);
    }
    
    void check_alive() const
    {
      this->method_.check_alive();
    }
    
    method_type & method() const
    {
      return this->method_;
    }

  private:
    method_type & method_;
  };
}

#endif // __VDS_CORE_METHOD_PROXY_H_
