#ifndef __VDS_CORE_FUNC_UTILS_H_
#define __VDS_CORE_FUNC_UTILS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  template <typename functor_type, typename functor_signature>
  struct _functor_info;

  template <typename functor_type, typename class_name, typename... arg_types>
  struct _functor_info<functor_type, void (class_name::*)(arg_types...)>
  {
    typedef void signature(arg_types...);
  };

  template <typename functor_type>
  struct functor_info : public _functor_info<functor_type, decltype(&functor_type::operator())>
  {};


  template <typename functor_type, typename functor_signature>
  class _lambda_handler;
  
  template <typename functor_type, typename class_name, typename... arg_types>
  class _lambda_handler<functor_type, void (class_name::*)(arg_types...)>
  {
  public:
    _lambda_handler(const functor_type & functor)
    : functor_(functor)
    {
    }
    
    void operator()(arg_types... args)
    {
      this->functor_(args...);
    }
    
  private:
    functor_type functor_;
  };
  
  template <typename functor_type, typename class_name, typename... arg_types>
  class _lambda_handler<functor_type, void (class_name::*)(arg_types...) const>
  {
  public:
    _lambda_handler(const functor_type & functor)
    : functor_(functor)
    {
    }
    
    void operator()(arg_types... args)
    {
      this->functor_(args...);
    }
    
  private:
    functor_type functor_;
  };
  
  template <typename functor_type>
  inline
  auto
  lambda_handler(const functor_type & f)
  -> _lambda_handler<functor_type, decltype(&functor_type::operator())>
  {
    return _lambda_handler<functor_type, decltype(&functor_type::operator())>(f);
  }
  
  class empty_handler
  {
  public:
    void operator()() const
    {
    }
  };
}

#endif//__VDS_CORE_FUNC_UTILS_H_
