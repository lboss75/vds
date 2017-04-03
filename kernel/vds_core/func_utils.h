#ifndef __VDS_CORE_FUNC_UTILS_H_
#define __VDS_CORE_FUNC_UTILS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  template <typename functor_type, typename functor_signature>
  struct _functor_info;

  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  struct _functor_info<functor_type, result (class_name::*)(arg_types...)>
  {
    typedef result signature(arg_types...);
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
      this->functor_(std::forward<arg_types>(args)...);
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
      this->functor_(std::forward<arg_types>(args)...);
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
  
  template<typename target_type, typename tuple_type, std::size_t current_num, std::size_t... nums>
  class _call_with : public _call_with<target_type, tuple_type, current_num - 1, current_num - 1, nums...>
  {
    using base_class = _call_with<target_type, tuple_type, current_num - 1, current_num - 1, nums...>;
  public:
    _call_with(target_type & target, const tuple_type & arguments)
    : base_class(target, arguments)
    {
    }
  };
  
  template<typename target_type, typename tuple_type, std::size_t... nums>
  class _call_with<target_type, tuple_type, 0, nums...>
  {
  public:
    _call_with(target_type & target, const tuple_type & arguments)
    {
      target(std::get<nums>(arguments)...);
    }
  };
  
  template<typename target_type, typename tuple_type>
  inline void call_with(target_type & target, const tuple_type & arguments)
  {
    _call_with<target_type, tuple_type, std::tuple_size<tuple_type>::value>(target, arguments);
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
}

#endif//__VDS_CORE_FUNC_UTILS_H_
