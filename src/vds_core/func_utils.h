#ifndef __VDS_CORE_FUNC_UTILS_H_
#define __VDS_CORE_FUNC_UTILS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <functional>

namespace vds {
  template <typename owner_type, typename method_type, typename method_signature>
  class _auto_cleaner;

  template <typename owner_type, typename method_type, typename class_name, typename... arg_types>
  class _auto_cleaner<owner_type, method_type, void(class_name::*)(arg_types...) const>
  {
  public:
    _auto_cleaner(
      owner_type * owner,
      const method_type & method)
    : owner_(owner), method_(method)
    {
    }
    
    void operator()(arg_types... args) const
    {
      this->method_(args...);
      delete this->owner_;
    }
    
  private:
    owner_type * owner_;
    const method_type & method_;
  };
  
  template<typename... functor_types>
  class _sequence_builder;
  
  template<typename functor_type>
  class _sequence_builder<functor_type>
  {
  public:
    _sequence_builder(const functor_type & functor)
    : functor_(functor)
    {
    }
    
    template <
      typename done_method_type,
      typename error_method_type
    >
    class handler
    : public functor_type::template handler<done_method_type, error_method_type>
    {
    public:
      handler(
        const done_method_type & done_method,
        const error_method_type & error_method,
        const _sequence_builder & builder
      ): functor_type::template handler<
          done_method_type,
          error_method_type>(
            done_method,
            error_method,
            builder.functor_)
      {
      }
    };
    
    
  private:
    functor_type functor_;
  };
  
  template<typename first_functor_type, typename... rest_functor_types>
  class _sequence_builder<first_functor_type, rest_functor_types...>
  : public _sequence_builder<rest_functor_types...>
  {
    using base = _sequence_builder<rest_functor_types...>;
  public:
    _sequence_builder(const first_functor_type & functor, rest_functor_types... rest_functors)
    : functor_(functor), base(rest_functors...)
    {
    }
   
    template <
      typename done_method_type,
      typename error_method_type
    >
    class handler 
    : public first_functor_type::template handler<
      typename base::template handler<done_method_type,error_method_type>,
      error_method_type>
    {
      using base_handler_type = typename first_functor_type::template handler<
      typename base::template handler<done_method_type,error_method_type>,
      error_method_type>;
    public:
      handler(
        const done_method_type & done_method,
        const error_method_type & error_method,
        const _sequence_builder & builder
      )
      : base_handler_(done_method, error_method, builder),
        base_handler_type(base_handler_, error_method, builder.functor_)
      {
      }
    private:
      typename base::template handler<done_method_type,error_method_type> base_handler_;
    };
    
  private:
    first_functor_type functor_;
  };
  
  template<typename... functor_types>
  class _sequence
  {
  public:
    _sequence(functor_types... functors)
    : builder_(functors...)
    {
    }
    
    template <
      typename done_method_type,
      typename error_method_type,
      typename... arg_types
    >
    void
    operator()(
      const done_method_type & done_method,
      const error_method_type & error_method,
      arg_types... args
    )
    {
      auto handler = new _sequence_runner<done_method_type, error_method_type>(
          done_method,
          error_method,
          this->builder_);
      (*handler)(args...);
    }
    
  private:
    _sequence_builder<functor_types...> builder_;
    
    template <
      typename done_method_type,
      typename error_method_type
      >
    class _sequence_runner
    : public _sequence_builder<functor_types...>
    ::template handler<
        _auto_cleaner<
          _sequence_runner<done_method_type, error_method_type>,
          done_method_type,
          decltype(&done_method_type::operator())>,
        _auto_cleaner<
          _sequence_runner<done_method_type, error_method_type>,
          error_method_type, 
          decltype(&error_method_type::operator())>>
    {
      using done_proxy_type = 
        _auto_cleaner<
          _sequence_runner,
          done_method_type,
          decltype(&done_method_type::operator())>;
      using error_proxy_type = 
        _auto_cleaner<
          _sequence_runner,
          error_method_type, 
          decltype(&error_method_type::operator())>;
      using base = 
        typename _sequence_builder<functor_types...>
        ::template handler<
          done_proxy_type,
          error_proxy_type>;
    public:
      _sequence_runner(
        const done_method_type & done_method,
        const error_method_type & error_method,
        _sequence_builder<functor_types...> & builder
      ) : base(done_proxy_, error_proxy_, builder),
        done_proxy_(this, done_method),
        error_proxy_(this, error_method)
      {
      }      
      
    private:
      _auto_cleaner<
        _sequence_runner,
        done_method_type,
        decltype(&done_method_type::operator())> done_proxy_;
      _auto_cleaner<
        _sequence_runner,
        error_method_type, 
        decltype(&error_method_type::operator())> error_proxy_;
    };
    
  };
  
  template<typename... functor_types>
  inline _sequence<functor_types...>
  sequence(functor_types... functors){
    return _sequence<functor_types...>(functors...);
  }
}

#endif//__VDS_CORE_FUNC_UTILS_H_
