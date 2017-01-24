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
  ////////////////////////////////////////////////////////////////
  template <
    typename next_step_type,
    typename error_method_type
  >
  class sequence_step_context
  {
  public:
    typedef next_step_type next_step_t;
    typedef error_method_type error_method_t;

    sequence_step_context(
      next_step_t & next,
      error_method_t & error
    ) : next_(next), error_(error)
    {
    }

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
      : next(context.next_),
      error(context.error_)
    {
    }
#if _DEBUG
    typedef _method_proxy<typename context_type::next_step_t, output_signature> next_step_t;
    typedef _method_proxy<typename context_type::error_method_t, void(std::exception *)> error_method_t;
#else
    typename context_type::next_step_t & next_step_t;
    typename context_type::error_method_t & error_method_t;
#endif

    next_step_t next;
    error_method_t error;
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

  template <typename owner_type, typename method_type>
  class auto_cleaner : public _auto_cleaner<owner_type, method_type, decltype(&method_type::operator())>
  {
    using base = _auto_cleaner<owner_type, method_type, decltype(&method_type::operator())>;
  public:
    auto_cleaner(
      owner_type * owner,
      const method_type & method)
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

  template<typename... functor_types>
  class _sequence_builder;

  template<typename functor_type>
  class _sequence_builder<functor_type>
  {
  public:
    typedef functor_type functor_t;

    _sequence_builder(const functor_type & functor)
      : functor_(functor)
    {
    }

    _sequence_builder(functor_type && functor)
    : functor_(functor)
    {
    }

    template <
      typename done_method_type,
      typename error_method_type
    >
    class holder
    {
    public:
      typedef typename functor_type::template handler<sequence_step_context<done_method_type, error_method_type>> sequence_step_t;

      holder(
        done_method_type & done_method,
        error_method_type & error_method,
        const _sequence_builder & args
      ) : step(sequence_step_context<done_method_type, error_method_type>(done_method, error_method), args.functor_)
      {
      }

      sequence_step_t step;
    };

  private:
    functor_type functor_;
  };
  
  template<typename functor_type, typename... rest_functor_types>
  class _sequence_builder<functor_type, rest_functor_types...>
  : public _sequence_builder<rest_functor_types...>
  {
    using base_class = _sequence_builder<rest_functor_types...>;
  public:
    _sequence_builder(const functor_type & functor, rest_functor_types... rest_functors)
      : functor_(functor), base_class(std::move(rest_functors)...)
    {
    }
    _sequence_builder(functor_type && functor, rest_functor_types... rest_functors)
    : functor_(functor), base_class(std::move(rest_functors)...)
    {
    }

    template <
      typename done_method_type,
      typename error_method_type
    >
    class holder : public base_class::template holder<done_method_type, error_method_type>
    {
      using base = typename base_class::template holder<done_method_type, error_method_type>;
    public:
      typedef typename functor_type::template handler<sequence_step_context<
        typename base::sequence_step_t, error_method_type>> sequence_step_t;

      holder(
        done_method_type & done_method,
        error_method_type & error_method,
        const _sequence_builder & args
      ) : base(done_method, error_method, args),
        step(sequence_step_context<base::sequence_step_t, error_method_type>(base::step, error_method), args.functor_)
      {
      }

      sequence_step_t step;
    };
    
  private:
    functor_type functor_;
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
        handler->first_step()(args...);
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
    void
    operator()(
      const done_method_type & done_method,
      const error_method_type & error_method,
      arg_types... args
    )
    {
      try {
        auto handler = new _sequence_func_runner<
          done_method_type,
          error_method_type
        >(
          done_method,
          error_method,
          this->builder_);
        handler->first_step()(args...);
      }
      catch (std::exception * ex) {
        error_method(ex);
      }
    }
    
  private:
    _sequence_builder<functor_types...> builder_;
    
    template <
      typename done_method_type,
      typename error_method_type
      >
    class _sequence_runner
    : public _sequence_builder<functor_types...>
    ::template holder<
        auto_cleaner<
          _sequence_runner<done_method_type, error_method_type>,
          done_method_type>,
        auto_cleaner<
          _sequence_runner<done_method_type, error_method_type>,
          error_method_type>
      >
    {
      using done_proxy_type = auto_cleaner<_sequence_runner, done_method_type>;
      using error_proxy_type = auto_cleaner<_sequence_runner, error_method_type>;

      using base = 
        typename _sequence_builder<functor_types...>
        ::template holder<
          done_proxy_type,
          error_proxy_type>;
    public:
      _sequence_runner(
        done_method_type & done_method,
        error_method_type & error_method,
        _sequence_builder<functor_types...> & builder
      ) : base(done_proxy_, error_proxy_, builder),
        done_proxy_(this, done_method),
        error_proxy_(this, error_method)
      {
      }

      typename base::sequence_step_t & first_step()
      {
        return this->step;
      }
      
    private:
      auto_cleaner<_sequence_runner, done_method_type> done_proxy_;
      auto_cleaner<_sequence_runner, error_method_type> error_proxy_;
    };
    
    template <
      typename done_method_type,
      typename error_method_type
      >
    class _sequence_func_runner
    : public _sequence_builder<functor_types...>
    ::template holder<
        auto_cleaner<
          _sequence_func_runner<done_method_type, error_method_type>,
          done_method_type>,
        auto_cleaner<
          _sequence_func_runner<done_method_type, error_method_type>,
          error_method_type>
      >
    {
      using done_proxy_type = auto_cleaner<_sequence_func_runner, done_method_type>;
      using error_proxy_type = auto_cleaner<_sequence_func_runner, error_method_type>;

      using base = 
        typename _sequence_builder<functor_types...>
        ::template holder<
          done_proxy_type,
          error_proxy_type>;
    public:
      _sequence_func_runner(
        const done_method_type & done_method,
        const error_method_type & error_method,
        _sequence_builder<functor_types...> & builder
      ) : base(done_proxy_, error_proxy_, builder),
      done_method_(done_method),
      error_method_(error_method),
        done_proxy_(this, done_method_),
        error_proxy_(this, error_method_)
      {
      }      

      typename base::sequence_step_t & first_step()
      {
        return this->step;
      }

    private:
      done_method_type done_method_;
      error_method_type error_method_;
      auto_cleaner<_sequence_func_runner, done_method_type> done_proxy_;
      auto_cleaner<_sequence_func_runner, error_method_type> error_proxy_;
    };
  };
  
  template<typename... functor_types>
  inline _sequence<functor_types...>
  sequence(functor_types... functors){
    return _sequence<functor_types...>(functors...);
  }
}

#endif//__VDS_CORE_FUNC_UTILS_H_
