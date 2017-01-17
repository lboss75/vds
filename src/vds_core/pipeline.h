#ifndef __VDS_CORE_PIPELINE_H_
#define __VDS_CORE_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "func_utils.h"

namespace vds {
  template<typename... functor_types>
  class _pipeline_builder;
  
  template<typename functor_type>
  class _pipeline_builder<functor_type>
  {
  public:
    _pipeline_builder(const functor_type & functor)
    : functor_(functor)
    {
    }
    
    template <
      typename done_method_type,
      typename next_method_type,
      typename error_method_type
    >
    class handler
    : public functor_type::template handler<
        done_method_type,
        next_method_type,
        error_method_type>
    {
    public:
      handler(
        done_method_type & done_method,
        next_method_type & next_method,
        error_method_type & error_method,
        const _pipeline_builder & builder
      ): functor_type::template handler<
          done_method_type,
          next_method_type,
          error_method_type>(
            done_method,
            next_method,
            error_method,
            builder.functor_)
      {
      }
    };    
    
  private:
    functor_type functor_;
  };
  
  template <
    typename done_method_type
  >
  class processed_caller
  {
  public:
    processed_caller(
      done_method_type & done
    ) : done_(done)
    {
    }
    
    void operator()() {
      this->done_.processed();
    }
    
  private:
    done_method_type & done_;
  };
  
  template<typename first_functor_type, typename... rest_functor_types>
  class _pipeline_builder<first_functor_type, rest_functor_types...>
  : public _pipeline_builder<rest_functor_types...>
  {
    using base = _pipeline_builder<rest_functor_types...>;
  public:
    _pipeline_builder(
      const first_functor_type & functor,
      rest_functor_types... rest_functors)
    : functor_(functor), base(rest_functors...)
    {
    }
    
    template <
      typename done_method_type,
      typename next_method_type,
      typename error_method_type
    >
    class handler 
    : public first_functor_type::template handler<
      done_method_type,
      typename base::template handler<
        processed_caller<
          handler<
            done_method_type,
            next_method_type,
            error_method_type>>,
        next_method_type,
        error_method_type>,
      error_method_type>
    {
      using base_handler_type = typename first_functor_type::template handler<
        done_method_type,
        typename base::template handler<
          processed_caller<
            handler<
              done_method_type,
              next_method_type,
              error_method_type>>,
            next_method_type,
            error_method_type>,
          error_method_type>;
    public:
      handler(
        done_method_type & done_method,
        next_method_type & next_method,
        error_method_type & error_method,
        const _pipeline_builder & builder
      )
      : processed_caller_(*this),
        base_handler_(processed_caller_, next_method, error_method, builder),
        base_handler_type(done_method, base_handler_, error_method, builder.functor_)
      {
      }
    private:
      processed_caller<handler> processed_caller_;
      typename base::template handler<
        processed_caller<handler>,
        next_method_type,
        error_method_type> base_handler_;
    };
    
  private:
    first_functor_type functor_;
  };
  
  template<typename first_functor_type, typename... rest_functor_types>
  class _pipeline_runner
  : public _pipeline_builder<rest_functor_types...>
  {
    using base = _pipeline_builder<rest_functor_types...>;
  public:
    _pipeline_runner(
      const first_functor_type & functor,
      rest_functor_types... rest_functors)
    : functor_(functor), base(rest_functors...)
    {
    }
   
    template <
      typename next_method_type,
      typename error_method_type
    >
    class handler 
    : public first_functor_type::template handler<
      typename base::template handler<
        handler<next_method_type, error_method_type>,
        auto_cleaner<
          handler<next_method_type, error_method_type>,
          next_method_type>,
        auto_cleaner<
          handler<next_method_type, error_method_type>,
          error_method_type>>,
      auto_cleaner<
        handler<next_method_type, error_method_type>,
        error_method_type>>
    {
      using next_proxy_type = 
        auto_cleaner<
          handler,
          next_method_type>;
      using error_proxy_type = 
        auto_cleaner<
          handler,
          error_method_type>;
      using base_handler_type = typename first_functor_type::template handler<
      typename base::template handler<handler,next_proxy_type,error_proxy_type>,
      error_proxy_type>;
    public:
      handler(
        next_method_type & next_method,
        error_method_type & error_method,
        const _pipeline_runner & builder
      )
      : next_proxy_(this, next_method),
        error_proxy_(this, error_method),
        base_handler_(*this, next_proxy_, error_proxy_, builder),
        base_handler_type(base_handler_, error_proxy_, builder.functor_)
      {
      }
    private:
      typename base::template handler<
        handler,
        next_proxy_type,
        error_proxy_type> base_handler_;
      auto_cleaner<
        handler,
        next_method_type> next_proxy_;
      auto_cleaner<
        handler,
        error_method_type> error_proxy_;
    };
    template <
      typename next_method_type,
      typename error_method_type
    >
    class func_handler 
    : public first_functor_type::template handler<
      typename base::template handler<
        func_handler<next_method_type, error_method_type>,
        auto_cleaner<
          func_handler<next_method_type, error_method_type>,
          next_method_type>,
        auto_cleaner<
          func_handler<next_method_type, error_method_type>,
          error_method_type>>,
      auto_cleaner<
        func_handler<next_method_type, error_method_type>,
        error_method_type>>
    {
      using next_proxy_type = 
        auto_cleaner<
          func_handler,
          next_method_type>;
      using error_proxy_type = 
        auto_cleaner<
          func_handler,
          error_method_type>;
      using base_handler_type = typename first_functor_type::template handler<
      typename base::template handler<func_handler,next_proxy_type,error_proxy_type>,
      error_proxy_type>;
    public:
      func_handler(
        const next_method_type & next_method,
        const error_method_type & error_method,
        const _pipeline_runner & builder
      )
      : next_method_(next_method),
        error_method_(error_method),
        next_proxy_(this, next_method_),
        error_proxy_(this, error_method_),
        base_handler_(*this, next_proxy_, error_proxy_, builder),
        base_handler_type(base_handler_, error_proxy_, builder.functor_)
      {
      }
    private:
      next_method_type next_method_;
      error_method_type error_method_;
      typename base::template handler<
        func_handler,
        next_proxy_type,
        error_proxy_type> base_handler_;
      auto_cleaner<
        func_handler,
        next_method_type> next_proxy_;
      auto_cleaner<
        func_handler,
        error_method_type> error_proxy_;
    };
    
  private:
    first_functor_type functor_;
  };
  
  template<typename... functor_types>
  class _pipeline
  {
  public:
    _pipeline(functor_types... functors)
    : builder_(functors...)
    {
    }
    
    template <
      typename done_method_type,
      typename error_method_type,
      typename... arg_types
    >
    void operator()(
      done_method_type & done_method,
      error_method_type & error_method,
      arg_types... args
    )
    {
      try {
        auto handler = new typename _pipeline_runner<functor_types...>::template handler<done_method_type, error_method_type>(
          done_method,
          error_method,
          this->builder_);
        (*handler)(args...);
      }
      catch (std::exception * ex) {
        error_method(ex);
      }
    }
    template <
      typename done_method_type,
      typename error_method_type,
      typename... arg_types
    >
    void operator()(
      const done_method_type & done_method,
      const error_method_type & error_method,
      arg_types... args
    )
    {
      try {
        auto handler = new typename _pipeline_runner<functor_types...>::template func_handler<done_method_type, error_method_type>(
          done_method,
          error_method,
          this->builder_);
        (*handler)(args...);
      }
      catch (std::exception * ex) {
        error_method(ex);
      }
    }
  private:
    _pipeline_runner<functor_types...> builder_;
  };
  
  template<typename... functor_types>
  inline _pipeline<functor_types...>
  pipeline(functor_types... functors){
    return _pipeline<functor_types...>(functors...);
  }
}

#endif // ! __VDS_CORE_PIPELINE_H_

