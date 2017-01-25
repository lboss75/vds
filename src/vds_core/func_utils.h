#ifndef __VDS_CORE_FUNC_UTILS_H_
#define __VDS_CORE_FUNC_UTILS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <functional>

namespace vds {
  ///////////////////////////////////////////////////////////
  template <typename method_type, typename method_signature>
  class _method_proxy;

  template <typename method_type, typename... arg_types>
  class _method_proxy<method_type, void(arg_types...)>
  {
  public:
    _method_proxy(method_type & method)
      : method_(method)
    {
    }

    void operator()(arg_types... args)
    {
      this->method_(args...);
    }

  private:
    method_type & method_;
  };
  ///////////////////////////////////////////////////////////
  template <typename method_type, typename method_signature>
  class _processed_method_proxy;

  template <typename method_type, typename class_name, typename... arg_types>
  class _processed_method_proxy<method_type, void (class_name::*)(arg_types...)>
  {
  public:
    _processed_method_proxy(method_type & method)
      : method_(method)
    {
    }

    void operator()(arg_types... args)
    {
      this->method_.processed(args...);
    }

  private:
    method_type & method_;
  };
  template <typename method_type, typename class_name, typename... arg_types>
  class _processed_method_proxy<method_type, void (class_name::*)(arg_types...) const>
  {
  public:
    _processed_method_proxy(method_type & method)
      : method_(method)
    {
    }

    void operator()(arg_types... args)
    {
      this->method_.processed(args...);
    }

  private:
    method_type & method_;
  };
  ////////////////////////////////////////////////////////////////
  class _fake {};
  template <
    typename next_step_type,
    typename error_method_type
  >
  class sequence_first_step_context
  {
  public:
    typedef _fake prev_step_t;
    typedef next_step_type next_step_t;
    typedef error_method_type error_method_t;

    sequence_first_step_context(
      next_step_t & next,
      error_method_t & error
    ) : next_(next), error_(error)
    {
    }

    prev_step_t prev_;
    next_step_t & next_;
    error_method_t & error_;
  };
  template <
    typename prev_step_type,
    typename next_step_type,
    typename error_method_type
  >
  class sequence_step_context
  {
  public:
    typedef prev_step_type prev_step_t;
    typedef next_step_type next_step_t;
    typedef error_method_type error_method_t;

    sequence_step_context(
      prev_step_type & prev,
      next_step_t & next,
      error_method_t & error
    ) : prev_(prev), next_(next), error_(error)
    {
    }

    prev_step_t & prev_;
    next_step_t & next_;
    error_method_t & error_;
  };
  ////////////////////////////////////////////////////////////////
  template<
    typename context_type,
    typename output_signature
  >
    class sequence_step
  {
  public:
    typedef sequence_step base;

    sequence_step(
      const context_type & context
    )
      : 
      prev(context.prev_),
      next(context.next_),
      error(context.error_)
    {
    }
#if _DEBUG
    typedef _method_proxy<typename context_type::next_step_t, output_signature> next_step_t;
    typedef _method_proxy<typename context_type::error_method_t, void(std::exception *)> error_method_t;
#else
    typedef typename context_type::next_step_t & next_step_t;
    typedef typename context_type::error_method_t & error_method_t;
#endif
    typedef _processed_method_proxy<typename context_type::prev_step_t, void(void)> prev_step_t;

    next_step_t next;
    error_method_t error;
    prev_step_t prev;

    void processed()
    {
      this->prev();
    }

  };
  ////////////////////////////////////////////////////////////////

  template <typename owner_type, typename method_type, typename method_signature>
  class _auto_cleaner;

  template <typename owner_type, typename method_type, typename class_name, typename... arg_types>
  class _auto_cleaner<owner_type, method_type, void(class_name::*)(arg_types...)>
  {
  public:
    _auto_cleaner(
      owner_type * owner,
      method_type & method)
    : owner_(owner), method_(method)
    {
    }
    
    void operator()(arg_types... args)
    {
      this->method_(args...);
      delete this->owner_;
    }
    
  private:
    owner_type * owner_;
    method_type & method_;
  };

  template <typename owner_type, typename method_type, typename class_name, typename... arg_types>
  class _auto_cleaner<owner_type, method_type, void(class_name::*)(arg_types...) const >
  {
  public:
    _auto_cleaner(
      owner_type * owner,
      method_type & method)
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
    method_type & method_;
  };

  template <typename owner_type, typename method_type>
  class auto_cleaner : public _auto_cleaner<owner_type, method_type, decltype(&method_type::operator())>
  {
    using base = _auto_cleaner<owner_type, method_type, decltype(&method_type::operator())>;
  public:
    auto_cleaner(
      owner_type * owner,
      method_type & method)
      : base(owner, method)
    {
    }
  };

  template <typename owner_type>
  class delete_this
  {
  public:
    delete_this(
      owner_type * owner)
      : owner_(owner)
    {
    }

    void operator()() const
    {
      delete this->owner_;
    }

  private:
    owner_type * owner_;
  };
  /////////////////////////
 
  template<std::size_t index, typename error_method_type, typename... functor_types>
  class _sequence_step_holder
  {
  public:
    typedef sequence_step_context<
      typename _sequence_step_holder<index + 1, error_method_type, functor_types...>::processed_step_t,
      typename _sequence_step_holder<index - 1, error_method_type, functor_types...>::step_handler_t,
      error_method_type> step_context_t;
      
    typedef typename std::tuple_element<
      sizeof...(functor_types) - index,
      std::tuple<functor_types...>>::type functor_type;
      
    typedef typename functor_type::template handler<step_context_t> step_handler_t;
    typedef _processed_method_proxy<step_handler_t, decltype(&step_handler_t::processed)> processed_step_t;
  };
  
  template<typename error_method_type, typename... functor_types>
  class _sequence_step_start_holder
  {
  public:
    typedef sequence_first_step_context<
      typename _sequence_step_holder<
        sizeof...(functor_types) - 1,
        error_method_type,
        functor_types...>::step_handler_t,
      error_method_type> step_context_t;
      
    typedef typename std::tuple_element<0, std::tuple<functor_types...>>::type functor_type;
    typedef typename functor_type::template handler<step_context_t> step_handler_t;
    typedef _processed_method_proxy<step_handler_t, decltype(&step_handler_t::processed)> processed_step_t;

  };
  
  //Final
  template<typename error_method_type, typename... functor_types>
  class _sequence_step_holder<0, error_method_type, functor_types...>
  {
  public:
  };
  
  ////////////////////
  template<std::size_t index, typename error_method_type, typename... functor_types>
  class _sequence_holder : public _sequence_holder<index - 1, error_method_type, functor_types...>
  {
    using base_class = _sequence_holder<index - 1, error_method_type, functor_types...>;
    using step_context_t = typename _sequence_step_holder<index, error_method_type, functor_types...>::step_context_t;
    using processed_step_t = typename _sequence_step_holder<index + 1, error_method_type, functor_types...>::processed_step_t;
  public:
    
    _sequence_holder(
      processed_step_t & prev,
      error_method_type & error_method,
      const std::tuple<functor_types...> & args
    )
    : 
      step(
        step_context_t(prev, base_class::step, error_method),
        std::get<sizeof...(functor_types) - index>(args)),
      processed(step),
      base_class(processed)      
    {
    }
    
    typename _sequence_step_holder<index, error_method_type, functor_types...>::step_handler_t step;
    typename _sequence_step_holder<index, error_method_type, functor_types...>::processed_step_t processed;
  };
  
  template<typename error_method_type, typename... functor_types>
  class _sequence_start_holder
  : public _sequence_holder<sizeof...(functor_types) - 1, error_method_type, functor_types...>
  {
    using base_class = _sequence_holder<sizeof...(functor_types) - 1, error_method_type, functor_types...>;
    using step_context_t = typename _sequence_start_holder<error_method_type, functor_types...>::step_context_t;
  public:
    
    _sequence_start_holder(
      error_method_type & error_method,
      const std::tuple<functor_types...> & args
    )
    : step(step_context_t(base_class::step, error_method), std::get<0>(args)),
      processed(step),
      base_class(processed)      
    {
    }
    
    typename _sequence_step_holder<
      sizeof...(functor_types) - 1,
      error_method_type,
      functor_types...>::step_handler_t step;
    typename _sequence_step_holder<
      sizeof...(functor_types) - 1,
      error_method_type,
      functor_types...>::processed_step_t processed;
  };
  
  template<typename error_method_type, typename... functor_types>
  class _sequence_holder<0, error_method_type, functor_types...>
  {
    using processed_step_t = typename _sequence_step_holder<0, error_method_type, functor_types...>::processed_step_t;
  public:
    
    _sequence_holder(
      processed_step_t & prev,
      error_method_type & error_method,
      const std::tuple<functor_types...> & args
    )
    : step(this)
    {
    }
    
    virtual ~_sequence_holder()
    {
    }
    
    delete_this<_sequence_holder> step;
  };

  template<typename... functor_types>
  class _sequence
  {
  public:
    _sequence(functor_types... functors)
    : builder_(std::make_tuple<functor_types...>(functors...))
    {
    }
    
    template <
      typename error_method_type,
      typename... arg_types
    >
    void
    operator()(
      error_method_type & error_method,
      arg_types... args
    )
    {
      try {
        auto handler = new _sequence_runner<error_method_type>(
          error_method,
          this->builder_);
        (*handler)(args...);
      }
      catch (std::exception * ex) {
        error_method(ex);
      }
    }
    
  private:
    std::tuple<functor_types...> builder_;

    template <
      typename error_method_type
      >
    class _sequence_runner
    : public _sequence_start_holder<
      auto_cleaner<_sequence_runner<error_method_type>, error_method_type>,
      functor_types...>
    {
      using error_proxy_type = auto_cleaner<_sequence_runner, error_method_type>;
      using base_class = _sequence_start_holder<error_proxy_type, functor_types...>;
    public:
      _sequence_runner(
        error_method_type & error_method,
        const std::tuple<functor_types...> & builder
      ) : base_class(error_proxy_, builder),
        error_proxy_(this, error_method)
      {
      }

    private:
      auto_cleaner<_sequence_runner, error_method_type> error_proxy_;
    };
  };
  
  template<typename... functor_types>
  inline _sequence<functor_types...>
  sequence(functor_types... functors){
    return _sequence<functor_types...>(functors...);
  }
  
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
  
}

#endif//__VDS_CORE_FUNC_UTILS_H_
