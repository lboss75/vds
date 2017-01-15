#ifndef __VDS_CORE_PIPELINE_H_
#define __VDS_CORE_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>

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
        const done_method_type & done_method,
        const next_method_type & next_method,
        const error_method_type & error_method,
        const _sequence_builder & builder
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
      const done_method_type & done
    ) : done_(done)
    {
    }
    
    void operator()() const {
      this->done_.processed();
    }
    
  private:
    const done_method_type & done_;
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
        processed_caller<handler>,
        next_method_type,
        error_method_type>,
      error_method_type>
    {
      using base_handler_type = typename first_functor_type::template handler<
      processed_caller<handler>,
      typename base::template handler<done_method_type,error_method_type>,
      error_method_type>;
    public:
      handler(
        const done_method_type & done_method,
        const next_method_type & next_method,
        const error_method_type & error_method,
        const _sequence_builder & builder
      )
      : processed_caller_(this),
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
  class _pipeline_runner<first_functor_type, rest_functor_types...>
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
        handler,
        next_method_type,
        error_method_type>,
      error_method_type>
    {
      using base_handler_type = typename first_functor_type::template handler<
      typename base::template handler<handler,next_method_type,error_method_type>,
      error_method_type>;
    public:
      handler(
        const next_method_type & next_method,
        const error_method_type & error_method,
        const _sequence_builder & builder
      )
      : base_handler_(*this, next_method, error_method, builder),
        base_handler_type(base_handler_, error_method, builder.functor_)
      {
      }
    private:
      typename base::template handler<
        handler,
        next_method_type,
        error_method_type> base_handler_;
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
      typename error_method_type
    >
    typename _pipeline_runner<functor_types...>::template handler<done_method_type, error_method_type>
    operator()(
      const done_method_type & done_method,
      const error_method_type & error_method
    )
    {
      return typename _sequence_builder<functor_types...>::template handler<done_method_type, error_method_type>(
          done_method,
          error_method,
          this->builder_);
    }
  private:
    _sequence_builder<functor_types...> builder_;
  };
  
  template<typename... functor_types>
  inline _sequence<functor_types...>
  sequence(functor_types... functors){
    return _sequence<functor_types...>(functors...);
  }
}

#endif // ! __VDS_CORE_PIPELINE_H_

