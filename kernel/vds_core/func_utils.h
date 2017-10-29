#ifndef __VDS_CORE_FUNC_UTILS_H_
#define __VDS_CORE_FUNC_UTILS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <functional>
#include <tuple>

namespace vds {
  template <typename functor_type, typename functor_signature>
  struct _functor_info;

  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  struct _functor_info<functor_type, result (class_name::*)(arg_types...)>
  {
    typedef result signature(arg_types...);
    
    template<template<typename...> typename target_template>
    struct build_type
    {
      typedef target_template<arg_types...> type;
    };
    
    typedef std::tuple<arg_types...> arguments_typle;
    typedef std::function<signature> function_type;
    typedef result result_type;
    
    static std::function<signature> to_function(functor_type & f)
    {
      return std::function<signature>([&f](arg_types... args){ f(args...);});
    }
    static std::function<signature> to_function(functor_type && f)
    {
      return std::function<signature>(f);
    }
  };

  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  struct _functor_info<functor_type, result(class_name::*)(arg_types...) const>
  {
    typedef result signature(arg_types...);
    typedef std::tuple<arg_types...> arguments_typle;
    typedef std::function<signature> function_type;
    typedef result result_type;
    
    template<template<typename...> typename target_template>
    struct build_type
    {
      typedef target_template<arg_types...> type;
    };

    static std::function<signature> to_function(functor_type & f)
    {
      return std::function<signature>([&f](arg_types ...args){ f(args...);});
    }
    
    static std::function<signature> to_function(functor_type && f)
    {
      return std::function<signature>(f);
    }
  };
  
  template <typename functor_type>
  struct functor_info : public _functor_info<functor_type, decltype(&functor_type::operator())>
  {};

  
  template<typename target_type, typename tuple_type, std::size_t current_num, std::size_t... nums>
  class _call_with : public _call_with<target_type, tuple_type, current_num - 1, current_num - 1, nums...>
  {
    using base_class = _call_with<target_type, tuple_type, current_num - 1, current_num - 1, nums...>;
  public:
    _call_with(const target_type & target, const std::shared_ptr<std::exception>& ex, tuple_type && arguments)
    : base_class(target, ex, std::move(arguments))
    {
    }
  };
  
  template<typename target_type, typename tuple_type, std::size_t... nums>
  class _call_with<target_type, tuple_type, 0, nums...>
  {
  public:
    _call_with(const target_type & target, const std::shared_ptr<std::exception>& ex, tuple_type && arguments)
    {
      target(ex, std::move(std::get<nums>(arguments))...);
    }
  };
  
  template<typename target_type, typename tuple_type>
  inline void call_with(const target_type & target, const std::shared_ptr<std::exception> & ex, tuple_type && arguments)
  {
    _call_with<target_type, tuple_type, std::tuple_size<tuple_type>::value>(target, ex, std::move(arguments));
  }
  
  class func_utils
  {
  public:
    template <typename functor>
    static inline typename functor_info<functor>::function_type to_function(functor & f)
    {
      return functor_info<functor>::to_function(f);
    }
  };

  template <typename first_parameter_type, typename func_signature>
  class add_first_parameter;
  
  template <typename first_parameter_type, typename result_type, typename... argument_types>
  class add_first_parameter<first_parameter_type, result_type(argument_types...)>
  {
  public:
    typedef result_type type(first_parameter_type, argument_types...);
  };
  
  ///////////////////////////////////////
  /*
  template <typename lambda_type, typename lambda_signature>
  class _lambda_holder;
  
  template <typename lambda_type>
  class lambda_holder : public _lambda_holder<lambda_type, decltype(&lambda_type::operator())>
  {
  };
  
  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  class _lambda_holder<functor_type, result (class_name::*)(arg_types...), std::enable_if<std::is_copy_constructible<functor_type>>
  {
  public:
    _lambda_holder(functor_type && f)
    : holder_(f)
    {
    }
    
    result operator()(arg_types... && args)
    {
      return this->holder_(std::forward<arg_types>(args)...);
    }
    
  protected:
    std::function<result(arg_types...)> holder_;
  };
  */  
}

#endif//__VDS_CORE_FUNC_UTILS_H_
