#ifndef __VDS_CORE_ASYNC_TASK_H_
#define __VDS_CORE_ASYNC_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "types.h"
#include <memory>

namespace vds {
 ////////////////////////////////////////////////////////////////
  template <typename function_signature>
  struct _async_task_arguments;

  template <typename done_method, typename class_name, typename ...argument_types>
  struct _async_task_arguments<void (class_name::*)(const done_method &, const error_handler &, argument_types ...)>
  {
    typedef done_method done_method_type;
    typedef void signature(const done_method_type &, const error_handler &, argument_types ...) ;
    typedef std::function<void(const done_method_type &, const error_handler &, argument_types ...)> type;
    typedef std::function<void(argument_types ...)> body;

    static body bind(const type & target, const done_method & done, const error_handler & on_error)
    {
      return [target, done, on_error](argument_types ... args) {
          target(done, on_error, args...);
      };
    }
    
    template<typename function_type>
    static type to_function(function_type f)
    {
      return type(f);
    }
  };

  template <typename done_method, typename class_name, typename ...argument_types>
  struct _async_task_arguments<void (class_name::*)(const done_method &, const error_handler &, argument_types ...) const>
  {
    typedef done_method done_method_type;
    typedef void signature(const done_method_type &, const error_handler &, argument_types ...) ;
    typedef std::function<void(const done_method_type &, const error_handler &, argument_types ...)> type;
    typedef std::function<void(argument_types ...)> body;

    static body bind(const type & target, const done_method & done, const error_handler & on_error)
    {
      return [target, done, on_error](argument_types ... args) {
          target(done, on_error, args...);
      };
    }
    
    template<typename function_type>
    static type to_function(function_type f)
    {
      return type(f);
    }
  };
  ////////////////////////////////////////////////////////////////
  template <typename new_done_method, typename functor>
  struct _async_task_replate_done_method;

  template <typename new_done_method, typename done_method_type, typename ...argument_types>
  struct _async_task_replate_done_method<new_done_method,
    std::function<void(done_method_type,
    const error_handler &,
    argument_types ...)>>
  {
    typedef std::function<void(done_method_type, const error_handler &, argument_types ...)> original;
    typedef std::function<void(new_done_method, const error_handler &, argument_types ...)> type;

    template<typename functor_type>
    static type join(const original & left, const functor_type & right)
    {
        return [left, right](const new_done_method & done, const error_handler & on_error, argument_types ... args) {
            left(_async_task_arguments<decltype(&functor_type::operator())>::bind(right, done, on_error), on_error, args...);
        };
    }
  };

  ////////////////////////////////////////////////////////////////
  template <typename ...functors>
  struct _async_task_of;

  ////////////////////////////////////////////////////////////////
  template <typename functor_type>
  class _async_task
  {
  public:
    _async_task(const functor_type & functor)
    : invoke(functor)
    {
    }
    
    
    template <typename functor_right>
    std::shared_ptr<
      _async_task<
        typename _async_task_replate_done_method<
          typename _async_task_arguments<decltype(&functor_right::operator())>::done_method_type,
          functor_type
         >::type
       >
    >
    then(const functor_right & right){
      return std::make_shared<
      _async_task<
        typename _async_task_replate_done_method<
          typename _async_task_arguments<decltype(&functor_right::operator())>::done_method_type,
          functor_type
        >::type>>(
        _async_task_replate_done_method<
          typename _async_task_arguments<decltype(&functor_right::operator())>::done_method_type,
          functor_type
        >::join(this->invoke, right));
    }
    
    functor_type invoke;
  };
  
  template <typename functor>
  inline
  std::shared_ptr<_async_task<typename _async_task_arguments<decltype(&functor::operator())>::type>>
  async_task(const functor & f)
  {
    return std::make_shared<_async_task<typename _async_task_arguments<decltype(&functor::operator())>::type>>(
      _async_task_arguments<decltype(&functor::operator())>::to_function(f)
    );
  }
}

#endif // __VDS_CORE_ASYNC_TASK_H_
 