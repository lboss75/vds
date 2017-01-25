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
  class _fake
  {
  public:
    void processed()
    {
    }    
  };
  
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
    ) : next_(next), error_(error), prev_(*(_fake*)nullptr)
    {
    }

    prev_step_t & prev_;
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
    typedef typename context_type::prev_step_t & prev_step_t;

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
  template<
    std::size_t index,
    typename tuple_type
    >
  using _functor_type_t = typename std::tuple_element<index, tuple_type>::type;

  
  template<
    std::size_t index,
    typename done_method_type,
    typename error_method_type,
    typename tuple_type,
    typename enabled = void>
  class _sequence_step_context;
  
  template<
    std::size_t index,
    typename done_method_type,
    typename error_method_type,
    typename tuple_type>
  class _sequence_step_handler
  : public _functor_type_t<index, tuple_type>::template handler<
    _sequence_step_context<
      index,
      done_method_type,
      error_method_type,
      tuple_type
    >
  >
  {
    using functor_type = _functor_type_t<index, tuple_type>;
    using context_type = _sequence_step_context<
        index,
        done_method_type,
        error_method_type,
        tuple_type
      >;
    using base_class = typename functor_type::template handler<context_type>;
  public:
    _sequence_step_handler(
      const context_type & context,
      const functor_type & args
    ) : base_class(context, args)
    {
    }    
  };

  template<
    std::size_t index,
    typename done_method_type,
    typename error_method_type,
    typename tuple_type>
  class _sequence_processed_step
  : public _processed_method_proxy<
      _sequence_step_handler<index, done_method_type, error_method_type, tuple_type>,
      decltype(&_sequence_step_handler<index, done_method_type, error_method_type, tuple_type>::processed)>
  {
    using base_class = _processed_method_proxy<
      _sequence_step_handler<index, done_method_type, error_method_type, tuple_type>,
      decltype(&_sequence_step_handler<index, done_method_type, error_method_type, tuple_type>::processed)>;
  public:
    _sequence_processed_step
    (
      _sequence_step_handler<index, done_method_type, error_method_type, tuple_type> & step
    ) : base_class(step)
    {
    }
  };
  
  template<
    typename done_method_type,
    typename error_method_type,
    typename tuple_type>
  class _sequence_step_context <
    0,
    done_method_type,
    error_method_type,
    tuple_type,
    typename std::enable_if<0 < std::tuple_size<tuple_type>::value - 1>::type
  > : public sequence_first_step_context<
        _sequence_step_handler<
          1,
          done_method_type,
          error_method_type,
          tuple_type>,
      error_method_type>
  {
  public:
    using base_class = sequence_first_step_context<
      _sequence_step_handler<
        1,
        done_method_type,
        error_method_type,
        tuple_type>,
    error_method_type>;

    _sequence_step_context(
      typename base_class::next_step_t & next,
      error_method_type & error_method
    ) : base_class(next, error_method)
    {
    }
  };

  template<
    std::size_t index,
    typename done_method_type,
    typename error_method_type,
    typename tuple_type>
  class _sequence_step_context <
    index,
    done_method_type,
    error_method_type,
    tuple_type,
    typename std::enable_if<index < std::tuple_size<tuple_type>::value - 1>::type
  > : public sequence_step_context<
      _sequence_processed_step<
        index - 1,
        done_method_type,
        error_method_type,
        tuple_type>,
      _sequence_step_handler<
        index + 1,
        done_method_type,
        error_method_type,
        tuple_type>,
      error_method_type>
  {
    using base_class = sequence_step_context<
      _sequence_processed_step<
        index - 1,
        done_method_type,
        error_method_type,
        tuple_type>,
      _sequence_step_handler<
        index + 1,
        done_method_type,
        error_method_type,
        tuple_type>,
      error_method_type>;
  public:
    _sequence_step_context(
      typename base_class::prev_step_t & prev,
      typename base_class::next_step_t & next,
      error_method_type & error
    ) : base_class(prev, next, error)
    {
    }
  };
  
  template<
    std::size_t index,
    typename done_method_type,
    typename error_method_type,
    typename tuple_type>
  class _sequence_step_context <
    index,
    done_method_type,
    error_method_type,
    tuple_type,
    typename std::enable_if<index == std::tuple_size<tuple_type>::value - 1>::type
  > : public sequence_step_context<
      _sequence_processed_step<
        index - 1,
        done_method_type,
        error_method_type,
        tuple_type>,
      done_method_type,
      error_method_type>
  {
    using base_class = sequence_step_context<
      _sequence_processed_step<
        index - 1,
        done_method_type,
        error_method_type,
        tuple_type>,
      done_method_type,
      error_method_type>;
  public:
    _sequence_step_context(
      typename base_class::prev_step_t & prev,
      typename base_class::next_step_t & next,
      error_method_type & error
    ): base_class(prev, next, error)
    {
    }
  };
  /////////////////////////////////////////
  template<
    std::size_t index,
    typename done_method_type,
    typename error_method_type,
    typename tuple_type>
  class _sequence_holder
  : public _sequence_holder<index - 1, done_method_type, error_method_type, tuple_type>
  {
    using base_class = _sequence_holder<
      index - 1,
      done_method_type,
      error_method_type,
      tuple_type>;
      
    using step_context_t = _sequence_step_context<
      std::tuple_size<tuple_type>::value - index,
      done_method_type,
      error_method_type,
      tuple_type>;
      
    using processed_step_t = _sequence_processed_step<
      std::tuple_size<tuple_type>::value - index - 1,
      done_method_type,
      error_method_type,
      tuple_type>;
  public:
    _sequence_holder(
      processed_step_t & prev,
      done_method_type & done,
      error_method_type & error_method,
      const tuple_type & args
    )
      :
      step(
        step_context_t(prev, base_class::step, error_method),
        std::get<std::tuple_size<tuple_type>::value - index>(args)),
      processed(step),
      base_class(processed, done, error_method, args)
    {
    }

    _sequence_step_handler<
      std::tuple_size<tuple_type>::value - index,
      done_method_type,
      error_method_type,
      tuple_type> step;
    _sequence_processed_step<
      std::tuple_size<tuple_type>::value - index,
      done_method_type,
      error_method_type,
      tuple_type> processed;
  };
  
  template<typename done_method_type, typename error_method_type, typename tuple_type>
  class _sequence_holder<
    0,
    done_method_type,
    error_method_type,
    tuple_type>
  {
    using processed_step_t = _sequence_processed_step<
      std::tuple_size<tuple_type>::value - 1,
      done_method_type,
      error_method_type,
      tuple_type>;
  public:

    _sequence_holder(
      processed_step_t & prev,
      done_method_type & done,
      error_method_type & error_method,
      const tuple_type & args
    )
      : step(done)
    {
    }

    virtual ~_sequence_holder()
    {
    }

    done_method_type & step;
  };

  ////////////////////
  
  template<
    typename done_method_type,
    typename error_method_type,
    typename tuple_type>
  class _sequence_start_holder
  : public _sequence_holder<
    std::tuple_size<tuple_type>::value - 1,
    done_method_type,
    error_method_type,
    tuple_type>
  {
    using base_class = _sequence_holder<
      std::tuple_size<tuple_type>::value - 1,
      done_method_type,
      error_method_type,
      tuple_type>;
  public:
    using step_context_t = _sequence_step_context<
      0,
      done_method_type,
      error_method_type,
      tuple_type>;
    
    _sequence_start_holder(
      done_method_type & done_method,
      error_method_type & error_method,
      const tuple_type & args
    )
    : step(step_context_t(base_class::step, error_method), std::get<0>(args)),
      processed(step),
      base_class(processed, done_method, error_method, args)      
    {
    }
    
    _sequence_step_handler<
      0,
      done_method_type,
      error_method_type,
      tuple_type> step;
    _sequence_processed_step<
      0,
      done_method_type,
      error_method_type,
      tuple_type> processed;
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
      done_method_type & done_method,
      error_method_type & error_method,
      arg_types... args
    )
    {
      try {
        auto handler = new _sequence_runner<done_method_type, error_method_type>(
          done_method,
          error_method,
          this->builder_);
        handler->step(args...);
      }
      catch (std::exception * ex) {
        error_method(ex);
      }
    }
    
  private:
    std::tuple<functor_types...> builder_;

    template <
      typename done_method_type,
      typename error_method_type
      >
    class _sequence_runner
    : public _sequence_start_holder<
        auto_cleaner<_sequence_runner<done_method_type, error_method_type>, done_method_type>,
        auto_cleaner<_sequence_runner<done_method_type, error_method_type>, error_method_type>,
        std::tuple<functor_types...>
      >
    {
      using done_proxy_type = auto_cleaner<_sequence_runner, done_method_type>;
      using error_proxy_type = auto_cleaner<_sequence_runner, error_method_type>;
      using base_class = _sequence_start_holder<
        done_proxy_type,
        error_proxy_type,
        std::tuple<functor_types...>>;
    public:
      _sequence_runner(
        done_method_type & done_method,
        error_method_type & error_method,
        const std::tuple<functor_types...> & builder
      ) : base_class(done_proxy_, error_proxy_, builder),
        done_proxy_(this, done_method),
        error_proxy_(this, error_method)
      {
      }

    private:
      auto_cleaner<_sequence_runner, done_method_type> done_proxy_;
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
