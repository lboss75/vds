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
  ////////////////////////////////////////////////////////////////
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
#endif
    typedef _processed_method_proxy<typename context_type::prev_step_t, decltype(&context_type::prev_step_t::processed)> prev_step_t;

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

    functor_type functor_;
  };

  template<typename functor_type, typename... rest_functor_types>
  class _sequence_builder<functor_type, rest_functor_types...>
    : public _sequence_builder<rest_functor_types...>
  {
    using base = _sequence_builder<rest_functor_types...>;
  public:
    typedef functor_type functor_t;

    _sequence_builder(const functor_type & functor, rest_functor_types... rest_functors)
      : functor_(functor), base(rest_functors...)
    {
    }

    _sequence_builder(functor_type && functor, rest_functor_types... rest_functors)
      : functor_(functor), base(rest_functors...)
    {
    }

    functor_type functor_;
  };


  template<typename prev_functor, typename last_functor, typename error_method_type, typename... rest_functor_types>
  class _sequence_holder;

  template<typename prev_functor, typename last_functor, typename error_method_type, typename functor_type>
  class _sequence_holder<prev_functor, last_functor, error_method_type, functor_type>
    : public functor_type::template handler<sequence_step_context<prev_functor,last_functor, error_method_type>>
  {
    using step_context = sequence_step_context<prev_functor, last_functor, error_method_type>;
    using sequence_step = typename functor_type::template handler<step_context>;
  public:
    _sequence_holder(
      prev_functor & prev,
      last_functor & last,
      error_method_type & error,
      const _sequence_builder<functor_type> & builder
    ) : sequence_step(step_context(prev, last, error), builder.functor_)
    {
    }
  };


  template<typename prev_functor, typename last_functor, typename error_method_type, typename functor_type, typename... rest_functor_types>
  class _sequence_holder<prev_functor, last_functor, error_method_type, functor_type, rest_functor_types...>
    : private _sequence_holder<
       /*this class as prev_functor:*/_sequence_holder<prev_functor, last_functor, error_method_type, functor_type, rest_functor_types...>,
      last_functor, error_method_type, rest_functor_types...>,
    public functor_type::template handler<
      sequence_step_context<
        prev_functor,
        /*base class as next filter*/_sequence_holder<
          /*this class as prev_functor:*/_sequence_holder<prev_functor, last_functor, error_method_type, functor_type, rest_functor_types...>,
          last_functor, error_method_type, rest_functor_types...>,
        error_method_type>>
  {
    using this_class = _sequence_holder<prev_functor, last_functor, error_method_type, functor_type, rest_functor_types...>;
    using base_class = _sequence_holder<this_class, last_functor, error_method_type, rest_functor_types...>;
    using step_context = sequence_step_context<prev_functor, base_class, error_method_type>;
    using sequence_step_t = typename functor_type::template handler<step_context>;
  public:
    _sequence_holder(
      prev_functor & prev,
      last_functor & last,
      error_method_type & error,
      const _sequence_builder<functor_type, rest_functor_types...> & builder
    ) : sequence_step_t(step_context(prev, *this, error), builder.functor_),
      base_class(*this, last, error, builder)
    {
    }
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
        (*handler)(args...);
      }
      catch (std::exception * ex) {
        error_method(ex);
      }
    }
    
  private:
    _sequence_builder<functor_types...> builder_;

    class fake_holder
    {
    public:
      void processed()
      {

      }
    };

    template <
      typename done_method_type,
      typename error_method_type
      >
    class _sequence_runner
    : public _sequence_holder<fake_holder, 
        auto_cleaner<_sequence_runner<done_method_type, error_method_type>, done_method_type>,
        auto_cleaner<_sequence_runner<done_method_type, error_method_type>, error_method_type>, functor_types...>
    {
      using done_proxy_type = auto_cleaner<_sequence_runner, done_method_type>;
      using error_proxy_type = auto_cleaner<_sequence_runner, error_method_type>;

      using base = _sequence_holder<fake_holder, done_proxy_type, error_proxy_type, functor_types...>;
    public:
      _sequence_runner(
        done_method_type & done_method,
        error_method_type & error_method,
        const _sequence_builder<functor_types...> & builder
      ) : base(fake_holder_, done_proxy_, error_proxy_, builder),
        done_proxy_(this, done_method),
        error_proxy_(this, error_method)
      {
      }

    private:
      fake_holder fake_holder_;
      auto_cleaner<_sequence_runner, done_method_type> done_proxy_;
      auto_cleaner<_sequence_runner, error_method_type> error_proxy_;
    };
  };
  
  template<typename... functor_types>
  inline _sequence<functor_types...>
  sequence(functor_types... functors){
    return _sequence<functor_types...>(functors...);
  }
}

#endif//__VDS_CORE_FUNC_UTILS_H_
