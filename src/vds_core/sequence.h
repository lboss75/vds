#ifndef __VDS_CORE_SEQUENCE_H_
#define __VDS_CORE_SEQUENCE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  //////////////////////////////////////////////////////
  template <
    typename method_type,
    typename method_signature>
  class _processed_method_proxy;

  template <
    typename method_type,
    typename class_name,
    typename... arg_types>
  class _processed_method_proxy<method_type,
    void (class_name::*)(arg_types...)>
  {
  public:
    _processed_method_proxy(method_type & method)
      : method_(method)
    {
    }

    void operator()(arg_types... args) const
    {
      this->method_.processed(args...);
    }

  private:
    method_type & method_;
  };
  
  template <
    typename method_type,
    typename class_name,
    typename... arg_types>
  class _processed_method_proxy<
    method_type,
    void (class_name::*)(arg_types...) const>
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
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  class _fake
  {
  public:
    void operator()() const
    {
    }    
  };
  
  //////////////////////////////////////////////////////
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
    typedef const typename context_type::prev_step_t & prev_step_t;

    next_step_t next;
    error_method_t error;
    prev_step_t prev;

    void processed()
    {
      this->prev();
    }
  };
  //////////////////////////////////////////////////////
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
  //////////////////////////
  class _suicide
  {
  public:
    virtual ~_suicide()
    {
    }
    void delete_this()
    {
      delete this;
    }
  };
  template <typename method_type, typename method_signature>
  class _auto_delete_trigger;

  template <typename method_type, typename class_name, typename... arg_types>
  class _auto_delete_trigger<method_type, void(class_name::*)(arg_types...)>
  {
  public:
    _auto_delete_trigger(
      _suicide * owner,
      method_type & method)
    : owner_(owner), method_(method)
    {
    }
    
    void operator()(arg_types... args)
    {
      this->method_(args...);
      this->owner_->delete_this();
    }
    
  private:
    _suicide * owner_;
    method_type & method_;
  };

  template <typename method_type, typename class_name, typename... arg_types>
  class _auto_delete_trigger<method_type, void(class_name::*)(arg_types...) const >
  {
  public:
    _auto_delete_trigger(
      _suicide * owner,
      method_type & method)
      : owner_(owner), method_(method)
    {
    }

    void operator()(arg_types... args) const
    {
      this->method_(args...);
      this->owner_->delete_this();
    }

  private:
    _suicide * owner_;
    method_type & method_;
  };

  template <typename method_type>
  class auto_delete_trigger : public _auto_delete_trigger<method_type, decltype(&method_type::operator())>
  {
    using base = _auto_delete_trigger<method_type, decltype(&method_type::operator())>;
  public:
    auto_delete_trigger(
      _suicide * owner,
      method_type & method)
      : base(owner, method)
    {
    }
  };
  /////////////////////////

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
    using tuple_type = std::tuple<functor_types...>;
    tuple_type builder_;

    template<
      typename done_method_type,
      typename error_method_type>
    class _sequence_start_holder : public _suicide
    {
    public:
      template <
        typename prev_step_type,
        typename next_step_type
      >
      class sequence_step_context
      {
      public:
        typedef next_step_type next_step_t;
        typedef error_method_type error_method_t;

        sequence_step_context(
          prev_step_type & prev,
          next_step_t & next,
          error_method_t & error
        ) : prev_(prev), next_(next), error_(error)
        {
        }
        
        typedef
        _processed_method_proxy<
          prev_step_type, 
          decltype(&prev_step_type::processed)>
        prev_step_t;

        prev_step_t prev_;
        next_step_t & next_;
        error_method_t & error_;
      };
      template <
        typename next_step_type
      >
      class _sequence_first_step_context
      {
      public:
        typedef _fake prev_step_t;
        typedef next_step_type next_step_t;
        typedef error_method_type error_method_t;

        _sequence_first_step_context(
          next_step_t & next,
          error_method_t & error
        ) : next_(next), error_(error), prev_(*(_fake*)nullptr)
        {
        }

        prev_step_t & prev_;
        next_step_t & next_;
        error_method_t & error_;
      };
      
      template<
        std::size_t index
        >
      using _functor_type_t = typename std::tuple_element<index, tuple_type>::type;

      
      template<
        std::size_t index,
        typename enabled = void,
        bool dummy = true>
      class _sequence_step_context;
      
      template<std::size_t index, bool dummy = true>
      class _sequence_holder;
      
      template<std::size_t index, bool dummy = true>
      class _sequence_step_handler
      : public _functor_type_t<index>::template handler<
        _sequence_step_context<
          index
        >
      >
      {
        using functor_type = _functor_type_t<index>;
        using context_type = _sequence_step_context<
            index
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

      template <bool dummy>
      class _sequence_step_context<
        0,
        typename std::enable_if<0 < std::tuple_size<tuple_type>::value - 1>::type,
        dummy
      > : public _sequence_first_step_context<
          _sequence_step_handler<1>
        >
      {
      public:
        using base_class = _sequence_first_step_context<
          _sequence_step_handler<
            1>>;

        _sequence_step_context(
          typename base_class::next_step_t & next,
          error_method_type & error_method
        ) : base_class(next, error_method)
        {
        }
      };

      template<std::size_t index, bool dummy>
      class _sequence_step_context<
        index,
        typename std::enable_if<index < std::tuple_size<tuple_type>::value - 1>::type,
        dummy
      > : public sequence_step_context<
          _sequence_step_handler<
            index - 1>,
          _sequence_step_handler<
            index + 1>
          >
      {
        using base_class = sequence_step_context<
          _sequence_step_handler<
            index - 1>,
          _sequence_step_handler<
            index + 1>>;
      public:
        _sequence_step_context(
          _sequence_step_handler<
            index - 1> & prev,
          typename base_class::next_step_t & next,
          error_method_type & error
        ) : base_class(prev, next, error)
        {
        }
      };
    
      template<std::size_t index, bool dummy>
      class _sequence_step_context <
        index,
        typename std::enable_if<index == std::tuple_size<tuple_type>::value - 1>::type,
        dummy
      > : public sequence_step_context<
          _sequence_step_handler<
            index - 1>,
          done_method_type>
      {
        using base_class = sequence_step_context<
          _sequence_step_handler<index - 1>,
          done_method_type>;
      public:
        _sequence_step_context(
          _sequence_step_handler<
            index - 1> & prev,
          typename base_class::next_step_t & next,
          error_method_type & error
        ): base_class(prev, next, error)
        {
        }
      };

      template<std::size_t index, bool dummy>
      class _sequence_holder
      : public _sequence_holder<index - 1>
      {
        using base_class = _sequence_holder<
          index - 1>;
          
        using step_context_t = _sequence_step_context<
          std::tuple_size<tuple_type>::value - index>;
          
        using processed_step_t = _sequence_step_handler<
          std::tuple_size<tuple_type>::value - index - 1>;
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
          base_class(step, done, error_method, args)
        {
        }

        _sequence_step_handler<
          std::tuple_size<tuple_type>::value - index> step;
      };

      template<bool dummy>
      class _sequence_holder<0, dummy>
      {
        using processed_step_t = _sequence_step_handler<
          std::tuple_size<tuple_type>::value - 1>;
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

        done_method_type & step;
      };
      
      using step_context_t = _sequence_step_context<0>;
      
      _sequence_start_holder(
        done_method_type & done_method,
        error_method_type & error_method,
        const tuple_type & args
      )
      : step(step_context_t(holder_.step, error_method), std::get<0>(args)),
        holder_(step, done_method, error_method, args)      
      {
      }
      
      _sequence_holder<
        std::tuple_size<tuple_type>::value - 1
        > holder_;
      _sequence_step_handler<
        0> step;
    };
    
    
    template <
      typename done_method_type,
      typename error_method_type
      >
    class _sequence_runner
    : public _sequence_start_holder<
        auto_delete_trigger<done_method_type>,
        auto_delete_trigger<error_method_type>
      >
    {
      using done_proxy_type = auto_delete_trigger<done_method_type>;
      using error_proxy_type = auto_delete_trigger<error_method_type>;
      using base_class = _sequence_start_holder<
        done_proxy_type,
        error_proxy_type>;
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
      auto_delete_trigger<done_method_type> done_proxy_;
      auto_delete_trigger<error_method_type> error_proxy_;
    };
  };
  
  template<typename... functor_types>
  inline _sequence<functor_types...>
  sequence(functor_types... functors){
    return _sequence<functor_types...>(functors...);
  }
  
  template <typename... arg_types>
  class for_each
  {
  public:
    
    template <typename handler_args_type>
    class _create_handler
    {
    public:
      _create_handler(const handler_args_type & handler_args)
      : handler_args_(handler_args)
      {
      }
      
      template <typename context_type>
      class handler
      {
      public:
        handler(
          const context_type & context,
          const _create_handler & args
        ) : prev(context.prev_), error_(context.error_),
        handler_args_(args.handler_args_)
        {
        }
        
        void operator ()(arg_types&... args)
        {
          auto handler = new typename handler_args_type::template handler
          <
            typename context_type::error_method_t
          >(
            this->error_,
            this->handler_args_,
            args...);
          handler->start();
          this->prev();
        }

        void processed()
        {
          this->prev();
        }
      private:
        typename context_type::error_method_t & error_;
        
        typedef const typename context_type::prev_step_t & prev_step_t;
        prev_step_t prev;
        handler_args_type handler_args_;
      };
      
    private:
      handler_args_type handler_args_;
    };
    
    template <typename handler_args_type>
    inline static auto create_handler(const handler_args_type & handler_args)
    -> _create_handler<handler_args_type>
    {
      return _create_handler<handler_args_type>(handler_args);
    }
  };
}

#endif // __VDS_CORE_SEQUENCE_H_
