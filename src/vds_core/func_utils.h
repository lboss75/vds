#ifndef __VDS_CORE_FUNC_UTILS_H_
#define __VDS_CORE_FUNC_UTILS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <functional>

namespace vds {
  typedef std::function<void(void)> simple_done_handler_t;
  typedef std::function<void(std::exception *)> error_handler_t;
  ////////////////////////////////////////////////////////////////
  template<typename T>
  struct memfun_type;

  template<typename Ret, typename Class, typename... Args>
  struct memfun_type<Ret(Class::*)(Args...) const>
  {
    typedef Ret signature(Args...);
    using type = std::function<Ret(Args...)>;
  };

  template<typename F>
  inline
  typename memfun_type<decltype(&F::operator())>::type
  to_function(const F & func)
  {
      return func;
  }
  ////////////////////////////////////////////////////////////////
  template <typename function_signature>
  struct _func_arguments;

  template <typename done_method, typename ...argument_types>
  struct _func_arguments<std::function<void (const done_method &, const error_handler_t &, argument_types ...)>>
  {
    typedef done_method done_method_type;
    typedef std::function<void(const done_method_type &, const error_handler_t &, argument_types ...)> type;
    typedef std::function<void(argument_types ...)> body;

    static body bind(const type & target, const done_method & done, const error_handler_t & error_handler)
    {
        return [target, done, error_handler](argument_types ... args) {
          try {
            target(done, error_handler, args...);
          }
          catch (std::exception * ex) {
            error_handler(ex);
          }
        };
    }
  };

  ////////////////////////////////////////////////////////////////
  template <typename new_done_method, typename functor>
  struct _func_replate_done_method;

  template <typename new_done_method, typename done_method_type, typename ...argument_types>
  struct _func_replate_done_method<new_done_method, std::function<void(done_method_type, const error_handler_t &, argument_types ...)>>
  {
    typedef std::function<void(done_method_type, const error_handler_t &, argument_types ...)> original;
    typedef std::function<void(new_done_method, const error_handler_t &, argument_types ...)> type;

    template<typename functor>
    static type join(const original & left, const functor & right)
    {
        return [left, right](const new_done_method & done, const error_handler_t & error_handler, argument_types ... args) {
          try {
            left(_func_arguments<functor>::bind(right, done, error_handler), error_handler, args...);
          }
          catch (std::exception * ex) {
            error_handler(ex);
          }
        };
    }
  };

  ////////////////////////////////////////////////////////////////
  template <typename ...functors>
  struct _func_of;

  template <typename functor_left, typename functor_right>
  struct _func_of<functor_left, functor_right>
  {
    typedef typename _func_replate_done_method<
      typename _func_arguments<functor_right>::done_method_type,
      functor_left
     >::type type;

     static type join(const functor_left & left, const functor_right & right)
     {
       typedef typename _func_arguments<functor_right>::done_method_type done_method;
        
       return _func_replate_done_method<
        done_method,
        functor_left>::join(left, right);
     }
  };

  ////////////////////////////////////////////////////////////////
  template <typename functor_left, typename functor_right>
  inline
  typename _func_of<
    typename memfun_type<decltype(&functor_left::operator())>::type,
    typename memfun_type<decltype(&functor_right::operator())>::type
    >::type
  sequence(const functor_left & left, const functor_right & right)
  {
      return _func_of<
        typename memfun_type<decltype(&functor_left::operator())>::type,
        typename memfun_type<decltype(&functor_right::operator())>::type
      >::join(
        to_function(left), to_function(right));
  }

  template <typename functor_left, typename functor_right, typename ... functor>
  inline auto 
  sequence(const functor_left & left, const functor_right & right, functor ... functors)
  -> decltype(sequence(_func_of<
        typename memfun_type<decltype(&functor_left::operator())>::type,
        typename memfun_type<decltype(&functor_right::operator())>::type
    >::join(to_function(left), to_function(right)), functors ...))
  {
    return sequence(_func_of<
      typename memfun_type<decltype(&functor_left::operator())>::type,
      typename memfun_type<decltype(&functor_right::operator())>::type
    >::join(to_function(left), to_function(right)), functors ...);
  }
}

#endif//__VDS_CORE_FUNC_UTILS_H_
