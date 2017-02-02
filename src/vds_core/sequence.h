#ifndef __VDS_CORE_SEQUENCE_H_
#define __VDS_CORE_SEQUENCE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "method_proxy.h"

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
    _processed_method_proxy(const _processed_method_proxy&) = delete;
    _processed_method_proxy(_processed_method_proxy&&) = delete;
    _processed_method_proxy & operator = (const _processed_method_proxy &) = delete;
    _processed_method_proxy & operator = (_processed_method_proxy &&) = delete;

    _processed_method_proxy(method_type & method)
      : method_(method)
    {
    }

    void operator()(arg_types... args) const
    {
      this->method_.check_alive();
      this->method_.processed(args...);
    }
    
    void check_alive() const
    {
      this->method_.check_alive();
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
    _processed_method_proxy(const _processed_method_proxy&) = delete;
    _processed_method_proxy(_processed_method_proxy&&) = delete;
    _processed_method_proxy & operator = (const _processed_method_proxy &) = delete;
    _processed_method_proxy & operator = (_processed_method_proxy &&) = delete;
    
    _processed_method_proxy(method_type & method)
      : method_(method)
    {
    }

    void operator()(arg_types... args)
    {
      this->method_.processed(args...);
    }
    
    void check_alive() const
    {
      this->method_.check_alive();
    }

  private:
    method_type & method_;
  };
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  class _fake
  {
  public:
    static _fake & instance()
    {
      static _fake s_instance;
      return s_instance;
    }
    void processed() const
    {
      throw new std::runtime_error("Logic invalid");
    }
    
    void check_alive()
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
    sequence_step(const sequence_step &) = delete;
    sequence_step(sequence_step &&) = delete;
    sequence_step & operator = (const sequence_step &) = delete;
    sequence_step & operator = (sequence_step &&) = delete;
    
    typedef sequence_step base;

    sequence_step(
      const context_type & context
    )
      : 
#ifdef DEBUG
      is_alive_sig_(0x37F49C0F),
#endif
      prev(context.prev_),
      next(context.next_),
      error(context.error_)
    {
    }
    
#ifdef DEBUG
    ~sequence_step()
    {
      this->is_alive_sig_ = 0x7F49C0F3;
    }
#endif

    void check_alive() const
    {
#ifdef DEBUG
      if(this->is_alive_sig_ != 0x37F49C0F){
        throw new std::runtime_error("Memory is corrupted");
      }
#endif
    }

#ifdef DEBUG
    typedef method_proxy<
      typename context_type::next_step_t,
      output_signature> next_step_t;
    typedef method_proxy<
      typename context_type::error_method_t,
      void(std::exception *)> error_method_t;
#else
    typedef typename context_type::next_step_t & next_step_t;
    typedef typename context_type::error_method_t & error_method_t;
#endif
    typedef
      _processed_method_proxy<
        typename context_type::prev_step_t, 
      decltype(&context_type::prev_step_t::processed)>
        prev_step_t;

#ifdef DEBUG
    int is_alive_sig_;
#endif
    prev_step_t prev;
    next_step_t next;
    error_method_t error;

    void processed()
    {
      this->prev();
    }
    
    void validate()
    {
      this->check_alive();
      this->prev.check_alive();
      this->next.check_alive();
      this->error.check_alive();      
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
    _suicide()
#ifdef DEBUG
    : is_alive_sig_(0x37F49C0F)
#endif
    {
    }
    
    virtual ~_suicide()
    {
#ifdef DEBUG
      this->is_alive_sig_ = 0x7F49C0F3;
#endif
    }
    
    void delete_this()
    {
      delete this;
    }
    

    void check_alive() const
    {
#ifdef DEBUG
      if(this->is_alive_sig_ != 0x37F49C0F){
        throw new std::runtime_error("Memory is corrupted");
      }
#endif
    }
    
    private:
#ifdef DEBUG
      int is_alive_sig_;
#endif
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
    
    void check_alive() const
    {
      this->owner_->check_alive();
    }
    
    method_type & method() const
    {
      return this->method_;
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

    void check_alive() const
    {
      this->owner_->check_alive();
    }
    
    method_type & method() const
    {
      return this->method_;
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
    void validate()
    {
      this->check_alive();
    }
  };
  
  template <typename method_type>
  class remove_auto_delete_trigger
  {
  public:
    typedef method_type type;
    
    remove_auto_delete_trigger(method_type & val)
    : value(val)
    {
    }
    
    method_type & value;
  };
  
  template <typename method_type>
  class remove_auto_delete_trigger<
    auto_delete_trigger<
      method_type
    >
  >
  {
  public:
    typedef method_type type;
    
    remove_auto_delete_trigger(auto_delete_trigger<method_type> & val)
    : value(val.method())
    {
    }
    
    method_type & value;
  };
  
  template <typename method_type, typename method_signature>
  class remove_auto_delete_trigger<
    method_proxy<
      auto_delete_trigger<
        method_type
      >,
      method_signature
    >
  >
  {
  public:
    typedef method_type type;
    
    remove_auto_delete_trigger(method_proxy<auto_delete_trigger<method_type>, method_signature> & val)
    : value(val.method().method())
    {
    }
    
    method_type & value;
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
        auto handler = new _sequence_runner<
          typename remove_auto_delete_trigger<done_method_type>::type,
          typename remove_auto_delete_trigger<error_method_type>::type
          >(
          remove_auto_delete_trigger<done_method_type>(done_method).value,
          remove_auto_delete_trigger<error_method_type>(error_method).value,
          this->builder_);
        handler->validate();
        handler->holder_.step(args...);
      }
      catch (std::exception * ex) {
        error_method(ex);
      }
    }

    template <
      typename done_method_type,
      typename error_method_type
    >
    class prepared_sequence
    {
    public:
      prepared_sequence(
        done_method_type & done_method,
        error_method_type & error_method,
        const std::tuple<functor_types...> & builder
      ) : holder_(done_method, error_method, builder)
      {

      }

    private:
      _sequence_start_holder<done_proxy_type, error_proxy_type> holder_;
    };


    template <
      typename done_method_type,
      typename error_method_type
    >
    prepared_sequence<done_method_type, error_method_type> *
    prepare(
        done_method_type & done_method,
        error_method_type & error_method
     )
    {
      auto handler = new prepared_sequence<done_method_type, error_method_type>(
        done_method,
        error_method,
        this->builder_);
      handler->validate();
      return handler;
    }

  private:
    using tuple_type = std::tuple<functor_types...>;
    tuple_type builder_;

    template<
      typename done_method_type,
      typename error_method_type>
    class _sequence_start_holder
    {
    public:
      template <
        typename prev_step_type,
        typename next_step_type
      >
      class sequence_step_context
      {
      public:
        typedef prev_step_type prev_step_t; 
        typedef next_step_type next_step_t;
        typedef error_method_type error_method_t;

        sequence_step_context(
          prev_step_t & prev,
          next_step_t & next,
          error_method_t & error
        ) : prev_(prev), next_(next), error_(error)
        {
        }
        
        prev_step_t & prev_;
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
        ) : prev_(_fake::instance()), next_(next), error_(error)
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
      {
        using holder_class = _sequence_holder<index - 1>;
          
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
          holder_(step, done, error_method, args),
          step(
            step_context_t(prev, holder_.step, error_method),
            std::get<std::tuple_size<tuple_type>::value - index>(args))
          
        {
        }
        
        void validate()
        {
          this->step.validate();
          this->holder_.validate();
        }

        holder_class holder_;
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
        
        void validate()
        {
          this->step.validate();
        }

        done_method_type & step;
      };
      
      using step_context_t = _sequence_step_context<0>;
      
      _sequence_start_holder(
        done_method_type & done_method,
        error_method_type & error_method,
        const tuple_type & args
      )
      : 
        holder_(step, done_method, error_method, args),        
        step(step_context_t(holder_.step, error_method), std::get<0>(args))
      {
      }
      
      void validate()
      {
        this->step.validate();
        this->holder_.validate();
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
    class _sequence_runner : public _suicide
    {
      using done_proxy_type = auto_delete_trigger<done_method_type>;
      using error_proxy_type = auto_delete_trigger<error_method_type>;
      using holder_class = _sequence_start_holder<
        done_proxy_type,
        error_proxy_type>;
    public:
      _sequence_runner(const _sequence_runner &) = delete;
      _sequence_runner(_sequence_runner&&) = delete;
      _sequence_runner & operator = (const _sequence_runner &) = delete;
      _sequence_runner & operator = (_sequence_runner&&) = delete;
      
      _sequence_runner(
        done_method_type & done_method,
        error_method_type & error_method,
        const std::tuple<functor_types...> & builder
      ) : 
        done_proxy_(this, done_method),
        error_proxy_(this, error_method),
        holder_(done_proxy_, error_proxy_, builder)
      {
      }
      
      void validate()
      {
        this->done_proxy_.check_alive();
        this->error_proxy_.check_alive();
        holder_.validate();
      }

      auto_delete_trigger<done_method_type> done_proxy_;
      auto_delete_trigger<error_method_type> error_proxy_;
      holder_class holder_;
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
        ) :
#ifdef DEBUG
        is_alive_sig_(0x37F49C0F),
#endif
        prev(context.prev_), error_(context.error_),
        handler_args_(args.handler_args_)
        {
        }
    
#ifdef DEBUG
        ~handler()
        {
          this->is_alive_sig_ = 0x7F49C0F3;
        }
#endif

        void check_alive() const
        {
#ifdef DEBUG
          if(this->is_alive_sig_ != 0x37F49C0F){
            throw new std::runtime_error("Memory is corrupted");
          }
#endif
        }
        
        void operator ()(arg_types&... args)
        {
          std::cout << "for_each()\n";
          try {
            auto handler = new typename handler_args_type::handler
            (
              this->handler_args_,
              args...);
            handler->start();
          }
          catch(std::exception * ex) {
            this->error_(ex);
          }
          
          this->prev();
        }

        void processed()
        {
          std::cout << "for_each::processed()\n";
          this->prev();
        }
        void validate()
        {
          this->prev.check_alive();
          this->error_.check_alive();
        }
      private:
#ifdef DEBUG
        int is_alive_sig_;
#endif
        _processed_method_proxy<
          typename context_type::prev_step_t, 
          decltype(&context_type::prev_step_t::processed)> prev;
        typename context_type::error_method_t & error_;
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
