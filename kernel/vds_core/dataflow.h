#ifndef __VDS_CORE_DATAFLOW_H_
#define __VDS_CORE_DATAFLOW_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "method_proxy.h"
#include "func_utils.h"
#include "types.h"
#include "deferred_callback.h"

namespace vds {
  class service_provider;

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

    void operator()(arg_types&&... args) const
    {
      this->method_.check_alive();
      this->method_.processed(std::forward<arg_types>(args)...);
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

    void operator()(arg_types&&... args)
    {
      this->method_.processed(std::forward<arg_types>(args)...);
    }
    
    void check_alive() const
    {
      this->method_.check_alive();
    }

  private:
    method_type & method_;
  };
  //////////////////////////////////////////////////////
  class _fake
  {
  public:
    static _fake & instance()
    {
      static _fake s_instance;
      return s_instance;
    }

    void processed(const service_provider & sp) const
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
    class dataflow_step
  {
  public:
    dataflow_step(const dataflow_step &) = delete;
    dataflow_step(dataflow_step &&) = delete;
    dataflow_step & operator = (const dataflow_step &) = delete;
    dataflow_step & operator = (dataflow_step &&) = delete;
    
    typedef dataflow_step base;

    dataflow_step(
      const context_type & context
    )
      : 
#ifdef DEBUG
      is_alive_sig_(0x37F49C0F),
#endif
      prev_proxy_(context.prev_),
      prev(context.deferred_context_, this->prev_proxy_),
      next(context.next_),
      error(context.error_)
    {
    }
    
#ifdef DEBUG
    ~dataflow_step()
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
      typename add_first_parameter<const service_provider &, output_signature>::type
    > next_step_t;
    typedef method_proxy<
      typename context_type::error_method_t,
      void(std::exception_ptr)> error_method_t;
#else
    typedef typename context_type::next_step_t & next_step_t;
    typedef typename context_type::error_method_t & error_method_t;
#endif
    typedef
      _processed_method_proxy<
        typename context_type::prev_step_t, 
      decltype(&context_type::prev_step_t::processed)>
        prev_step_real_t;
    typedef
      deferred_callback<prev_step_real_t>
      prev_step_t;

#ifdef DEBUG
    int is_alive_sig_;
#endif
    prev_step_real_t prev_proxy_;
    prev_step_t prev;
    next_step_t next;
    error_method_t error;

    void processed(const service_provider & sp)
    {
      this->prev(sp);
    }
    
    void validate()
    {
      this->check_alive();
      this->prev_proxy_.check_alive();
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
      this->method_(std::forward<arg_types>(args)...);
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
      this->method_(std::forward<arg_types>(args)...);
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
      this->method_(std::forward<arg_types>(args)...);
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
      this->method_(std::forward<arg_types>(args)...);
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
  ///////////////////////////////////////////////
  template <typename... argument_types>
  class _dataflow_starter
  {
  public:

    template <typename context_type>
    class handler : public dataflow_step<context_type, void(argument_types...)>
    {
      using base_class = dataflow_step<context_type, void(argument_types...)>;
    public:
      handler(
        const context_type & context,
        const _dataflow_starter & args)
        : base_class(context)
      {
      }

      void operator()(const service_provider & sp, argument_types... args)
      {
        this->next(sp, args...);
      }

      void processed(const service_provider & sp)
      {
        this->next(sp, argument_types()...);
      }
    };
  };
  ///////////////////////////////////////////////

  template<typename... functor_types>
  class _dataflow
  {
  public:
    _dataflow(functor_types&&... functors)
    : builder_(std::forward<functor_types>(functors)...)
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
      const service_provider & sp,
      arg_types... args
    )
    {
      try {
        auto handler = new _dataflow_runner<
          typename remove_auto_delete_trigger<done_method_type>::type,
          typename remove_auto_delete_trigger<error_method_type>::type,
          arg_types...
          >(
          remove_auto_delete_trigger<done_method_type>(done_method).value,
          remove_auto_delete_trigger<error_method_type>(error_method).value,
          this->builder_);
        handler->validate();
        handler->holder_.step(sp, args...);
      }
      catch (...) {
        error_method(std::current_exception());
      }
    }
    
    template <
      typename done_method_type,
      typename error_method_type,
      typename... arg_types
    >
    void
    operator()(
      done_method_type && done_method,
      error_method_type && error_method,
      const service_provider & sp,
      arg_types... args
    )
    {
      error_method_type error_handler(error_method);
      try {
        auto handler = new _dataflow_func_runner<
          typename std::remove_reference<done_method_type>::type,
          typename std::remove_reference<error_method_type>::type,
          arg_types...
          >(
          std::move(done_method),
          std::move(error_method),
          this->builder_);
        handler->validate();
        handler->holder_.step(sp, args...);
      }
      catch (...) {
        error_handler(std::current_exception());
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
      const service_provider & sp,
      arg_types... args
    )
    {
      error_method_type error_handler(error_method);
      try {
        auto handler = new _dataflow_func_runner<
          done_method_type,
          error_method_type,
          arg_types...
          >(
          done_method,
          error_method,
          this->builder_);
        handler->validate();
        handler->holder_.step(sp, args...);
      }
      catch (...) {
        error_handler(std::current_exception());
      }
    }

  private:
    using tuple_type = std::tuple<functor_types...>;
    tuple_type builder_;

    template<
      typename done_method_type,
      typename error_method_type,
      typename... arg_types>
    class _dataflow_start_holder
    {
      using tuple_type = std::tuple<_dataflow_starter<arg_types...>, functor_types...>;
    public:
      template<std::size_t index, bool dummy = true>
      class _dataflow_step_handler;

      class _dataflow_first_step_context
      {
      public:
        typedef _fake prev_step_t;
        typedef _dataflow_step_handler<1> next_step_t;
        typedef auto_delete_trigger<error_method_type> error_method_t;

        _dataflow_first_step_context(
          deferred_context & context,
          next_step_t & next,
          error_method_t & error
        ) : deferred_context_(context),
          prev_(_fake::instance()),
          next_(next),
          error_(error)
        {
        }

        deferred_context & deferred_context_;
        prev_step_t & prev_;
        next_step_t & next_;
        error_method_t & error_;
      };
      
      template<
        std::size_t index
        >
      using _functor_type_t = typename std::remove_reference<typename std::tuple_element<index, tuple_type>::type>::type;

      
      template<
        std::size_t index,
        typename enabled = void,
        bool dummy = true>
      class _dataflow_step_context;
      
      template<std::size_t index, bool dummy = true>
      class _dataflow_holder;
      
      template<std::size_t index, bool dummy>
      class _dataflow_step_handler
      : public _functor_type_t<index>::template handler<
        _dataflow_step_context<
          index
        >
      >
      {
        using functor_type = _functor_type_t<index>;
        using context_type = _dataflow_step_context<
            index
          >;
        using base_class = typename functor_type::template handler<context_type>;
      public:
        _dataflow_step_handler(
          const context_type & context,
          const functor_type & args
        ) : base_class(context, args)
        {
        }    
      };

      template <bool dummy>
      class _dataflow_step_context<
        0,
        typename std::enable_if<0 < std::tuple_size<tuple_type>::value - 1>::type,
        dummy
      > : public _dataflow_first_step_context
      {
      public:
        using base_class = _dataflow_first_step_context;

        _dataflow_step_context(
          deferred_context & context,
          typename base_class::next_step_t & next,
          auto_delete_trigger<error_method_type> & error_method
        ) : base_class(context, next, error_method)
        {
        }
      };

      template<std::size_t index, bool dummy>
      class _dataflow_step_context<
        index,
        typename std::enable_if<index < std::tuple_size<tuple_type>::value - 1>::type,
        dummy
      >
      {
      public:
        typedef _dataflow_step_handler<index - 1> prev_step_t;
        typedef _dataflow_step_handler<index + 1> next_step_t;
        typedef auto_delete_trigger<error_method_type> error_method_t;

        _dataflow_step_context(
          deferred_context & context,
          prev_step_t & prev,
          next_step_t & next,
          error_method_t & error
        ) : deferred_context_(context),
          prev_(prev), next_(next), error_(error)
        {
        }

        deferred_context & deferred_context_;
        prev_step_t & prev_;
        next_step_t & next_;
        error_method_t & error_;
      };

      template<std::size_t index, bool dummy>
      class _dataflow_step_context <
        index,
        typename std::enable_if<index == std::tuple_size<tuple_type>::value - 1>::type,
        dummy
      >
      {
      public:
        typedef _dataflow_step_handler<index - 1> prev_step_t;
        typedef auto_delete_trigger<done_method_type> next_step_t;
        typedef auto_delete_trigger<error_method_type> error_method_t;


        _dataflow_step_context(
          deferred_context & context,
          prev_step_t & prev,
          next_step_t & next,
          error_method_t & error
        ) : deferred_context_(context),
          prev_(prev),
          next_(next),
          error_(error)
        {
        }

        deferred_context & deferred_context_;
        prev_step_t & prev_;
        next_step_t & next_;
        error_method_t & error_;

      };

      template<std::size_t index, bool dummy>
      class _dataflow_holder
      {
        using holder_class = _dataflow_holder<index - 1>;
          
        using step_context_t = _dataflow_step_context<
          std::tuple_size<tuple_type>::value - index>;
          
        using processed_step_t = _dataflow_step_handler<
          std::tuple_size<tuple_type>::value - index - 1>;
      public:
        _dataflow_holder(
          deferred_context & context,
          processed_step_t & prev,
          auto_delete_trigger<done_method_type> & done,
          auto_delete_trigger<error_method_type> & error_method,
          const std::tuple<functor_types...> & args
        )
          :
          holder_(context, step, done, error_method, args),
          step(
            step_context_t(context, prev, holder_.step, error_method),
            std::get<std::tuple_size<tuple_type>::value - index - 1>(args))
        {
        }
        
        void validate()
        {
          this->step.validate();
          this->holder_.validate();
        }

        holder_class holder_;
        _dataflow_step_handler<
          std::tuple_size<tuple_type>::value - index> step;
      };

      template<bool dummy>
      class _dataflow_holder<0, dummy>
      {
        using processed_step_t = _dataflow_step_handler<
          std::tuple_size<tuple_type>::value - 1>;
      public:

        _dataflow_holder(
          deferred_context & context,
          processed_step_t & /*prev*/,
          auto_delete_trigger<done_method_type> & done,
          auto_delete_trigger<error_method_type> & /*error_method*/,
          const std::tuple<functor_types...> & /*args*/
        )
          : step(done)
        {
        }
        
        void validate()
        {
          this->step.validate();
        }

        auto_delete_trigger<done_method_type> & step;
      };
      
      using step_context_t = _dataflow_step_context<0>;
      
      _dataflow_start_holder(
        deferred_context & context,
        auto_delete_trigger<done_method_type> & done_method,
        auto_delete_trigger<error_method_type> & error_method,
        const std::tuple<functor_types...> & args
      )
      : 
        holder_(context, step, done_method, error_method, args),        
        step(step_context_t(context, holder_.step, error_method), _dataflow_starter<arg_types...>())
      {
      }
      
      void validate()
      {
        this->step.validate();
        this->holder_.validate();
      }
      
      _dataflow_holder<
        std::tuple_size<tuple_type>::value - 1
        > holder_;
      _dataflow_step_handler<
        0> step;
    };
    
    
    template <
      typename done_method_type,
      typename error_method_type,
      typename... arg_types>
    class _dataflow_runner : public _suicide
    {
      using tuple_type = std::tuple<_dataflow_starter<arg_types...>, functor_types...>;
      using done_proxy_type = auto_delete_trigger<done_method_type>;
      using error_proxy_type = auto_delete_trigger<error_method_type>;
      using holder_class = _dataflow_start_holder<done_method_type, error_method_type, arg_types...>;
    public:
      _dataflow_runner(const _dataflow_runner &) = delete;
      _dataflow_runner(_dataflow_runner&&) = delete;
      _dataflow_runner & operator = (const _dataflow_runner &) = delete;
      _dataflow_runner & operator = (_dataflow_runner&&) = delete;
      
      _dataflow_runner(
        done_method_type & done_method,
        error_method_type & error_method,
        const std::tuple<functor_types...> & builder
      ) : 
        context_(new deferred_context()),
        done_proxy_(this, done_method),
        error_proxy_(this, error_method),
        holder_(*this->context_, done_proxy_, error_proxy_, builder)
      {
      }
      
      void validate()
      {
        this->done_proxy_.check_alive();
        this->error_proxy_.check_alive();
        holder_.validate();
      }

      std::shared_ptr<deferred_context> context_;
      auto_delete_trigger<done_method_type> done_proxy_;
      auto_delete_trigger<error_method_type> error_proxy_;
      holder_class holder_;
    };
    
    template <
      typename done_method_type,
      typename error_method_type,
      typename... arg_types
      >
    class _dataflow_func_runner : public _suicide
    {
      using done_proxy_type = auto_delete_trigger<done_method_type>;
      using error_proxy_type = auto_delete_trigger<error_method_type>;
      using holder_class = _dataflow_start_holder<done_method_type, error_method_type, arg_types...>;
    public:
      _dataflow_func_runner(const _dataflow_func_runner &) = delete;
      _dataflow_func_runner(_dataflow_func_runner&&) = delete;
      _dataflow_func_runner & operator = (const _dataflow_func_runner &) = delete;
      _dataflow_func_runner & operator = (_dataflow_func_runner&&) = delete;
      
      _dataflow_func_runner(
        done_method_type && done_method,
        error_method_type && error_method,
        const std::tuple<functor_types...> & builder
      ) :
        context_(new deferred_context()),
        done_method_(done_method),
        error_method_(error_method),
        done_proxy_(this, done_method_),
        error_proxy_(this, error_method_),
        holder_(*this->context_, done_proxy_, error_proxy_, builder)
      {
      }
      
      _dataflow_func_runner(
        const done_method_type & done_method,
        const error_method_type & error_method,
        const std::tuple<functor_types...> & builder
      ) : 
        context_(new deferred_context()),
        done_method_(done_method),
        error_method_(error_method),
        done_proxy_(this, done_method_),
        error_proxy_(this, error_method_),
        holder_(*this->context_, done_proxy_, error_proxy_, builder)
      {
      }
      
      void validate()
      {
        this->done_proxy_.check_alive();
        this->error_proxy_.check_alive();
        holder_.validate();
      }

      std::shared_ptr<deferred_context> context_;
      done_method_type done_method_;
      error_method_type error_method_;
      auto_delete_trigger<done_method_type> done_proxy_;
      auto_delete_trigger<error_method_type> error_proxy_;
      holder_class holder_;
    };
  };
  
  template<typename... functor_types>
  inline _dataflow<functor_types...>
  dataflow(functor_types&&... functors){
    return _dataflow<functor_types...>(std::forward<functor_types>(functors)...);
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
        
        void operator ()(arg_types... args)
        {
          try {
            auto handler = new typename handler_args_type::handler
            (
              this->handler_args_,
              std::forward<arg_types>(args)...);
            handler->start();
          }
          catch(...) {
            this->error_(std::current_exception());
          }
          
          this->prev();
        }

        void processed(const service_provider & sp)
        {
          this->prev(sp);
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

  class multi_handler
  {
  public:
    multi_handler(
      size_t count,
      const std::function<void(std::list<std::exception_ptr> & errors)> & target)
      : target_(target),
      count_(count),
      done_(this),
      error_(this)
    {
    }

    class done_handler
    {
    public:
      done_handler(multi_handler * owner)
        : owner_(owner)
      {
      }

      void operator()()
      {
        this->owner_->done();
      }

    private:
      multi_handler * owner_;
    };

    done_handler & on_done() { return this->done_; }

    class error_handler
    {
    public:
      error_handler(multi_handler * owner)
        : owner_(owner)
      {
      }

      void operator()(std::exception_ptr ex)
      {
        this->owner_->error(ex);
      }

    private:
      multi_handler * owner_;
    };

    error_handler & on_error() { return this->error_; }

  protected:
    std::function<void(std::list<std::exception_ptr> & errors)> target_;
    std::list<std::exception_ptr> errors_;

    size_t count_;
    done_handler done_;
    error_handler error_;

    std::mutex data_mutex_;

    void done()
    {
      std::lock_guard<std::mutex> lock(this->data_mutex_);

      if (0 == --this->count_) {
        this->target_(this->errors_);
      }
    }

    void error(std::exception_ptr ex)
    {
      std::lock_guard<std::mutex> lock(this->data_mutex_);
      this->errors_.push_back(ex);

      if (0 == --this->count_) {
        this->target_(this->errors_);
      }
    }
  };

  template<typename handler_type, typename handler_signature>
  class _simple_step;

  template<typename handler_type, typename class_name, typename prev_handler_type, typename next_handler_type, typename... argument_types>
  class _simple_step<handler_type, void (class_name::*)(const prev_handler_type & prev, const next_handler_type & next, argument_types... arguments)>
  {
  public:
    _simple_step(const handler_type & handler)
      : handler_(handler)
    {
    }

    template<typename context_type>
    class handler : public dataflow_step<context_type, typename functor_info<next_handler_type>::signature>
    {
      using base_class = dataflow_step<context_type, typename functor_info<next_handler_type>::signature>;
    public:
      handler(
        const context_type & context,
        const _simple_step & args)
      : base_class(context),
        handler_(args.handler_)
      {
      }

      void operator()(const service_provider & sp, argument_types&&... arguments)
      {
        this->handler_(sp, prev_handler_type(this->prev), next_handler_type(this->next), std::forward(arguments)...);
      }

    private:
      handler_type handler_;
    };

  private:
    handler_type handler_;
  };

  template<typename handler_type>
  inline _simple_step<handler_type, decltype(&handler_type::operator())> simple_step(const handler_type & handler)
  {
    return _simple_step<handler_type, decltype(&handler_type::operator())>(handler);
  }

  template <template<typename> typename handler_class, typename... argument_types>
  class _step_container
  {
  public:
    _step_container(argument_types&&... args)
      : args_(std::forward<argument_types>(args)...)
    {
    }

    template <typename context_type, size_t current_num, size_t... nums>
    class handler_builder : public handler_builder<context_type, current_num - 1, current_num - 1, nums...>
    {
    public:
      handler_builder(
        const context_type & context,
        const std::tuple<argument_types...> & args)
        : handler_builder<context_type, current_num - 1, current_num - 1, nums...>(context, args)
      {
      }
    };

    template <typename context_type, size_t... nums>
    class handler_builder<context_type, 0, nums...> : public handler_class<context_type>
    {
    public:
      handler_builder(
        const context_type & context,
        const std::tuple<argument_types...> & args)
        : handler_class<context_type>(context, std::get<nums>(args)...)
      {
      }
    };

    template <typename context_type>
    class handler : public handler_builder<context_type, sizeof...(argument_types)>
    {
    public:
      handler(
      const context_type & context,
      const _step_container & args)
        : handler_builder<context_type, sizeof...(argument_types)>(context, args.args_)
      {
      }
    };

  private:
    std::tuple<argument_types...> args_;
  };

  template <template<typename> typename handler_class>
  struct create_step
  {
    template <typename... argument_types>
    static _step_container<handler_class, argument_types...> with(argument_types&&... args)
    {
      return _step_container<handler_class, argument_types...>(std::forward<argument_types&&>(args)...);
    }
  };
  
  template<typename signature>
  class _task_step;
  
  template<typename result_signature, typename class_name, typename... arg_types>
  class _task_step<void(class_name::*)(
    const std::function<result_signature> & done,
    const error_handler & on_error,
    const std::function<void(void)> & prev,
    arg_types...)>
  {
  public:
    using functor_type = std::function<void(
      const std::function<result_signature> & done,
      const error_handler & on_error,
      const std::function<void(void)> & prev,
      arg_types ...)>;
    _task_step(
      const functor_type & target)
    : target_(target)
    {
    }
    
    template<typename context_type>
    class handler : public dataflow_step<context_type, result_signature>
    {
      using base_class = dataflow_step<context_type, result_signature>;
    public:
      handler(
        const context_type & context,
        const _task_step & args)
      : base_class(context),
      target_(args.target_)
      {
      }
      
      void operator()(const service_provider & sp, arg_types... args)
      {
        this->target_(sp, this->prev, this->error, this->prev, args...);
      }
      
    private:
      functor_type target_;
    };
    
  private:
    functor_type target_;
  };
  
  template<typename result_signature, typename class_name, typename... arg_types>
  class _task_step<void(class_name::*)(
    const std::function<result_signature> & done,
    const error_handler & on_error,
    const std::function<void(void)> & prev,
    arg_types...) const>
  {
  public:
    using functor_type = std::function<void(
      const std::function<result_signature> & done,
      const error_handler & on_error,
      const std::function<void(void)> & prev,
      arg_types ...)>;
    _task_step(
      const functor_type & target)
    : target_(target)
    {
    }
    
    template<typename context_type>
    class handler : public dataflow_step<context_type, result_signature>
    {
      using base_class = dataflow_step<context_type, result_signature>;
    public:
      handler(
        const context_type & context,
        const _task_step & args)
      : base_class(context),
      target_(args.target_)
      {
      }
      
      void operator()(const service_provider & sp, arg_types... args)
      {
        this->target_(
          sp,
          func_utils::to_function(this->next),
          func_utils::to_function(this->error),
          func_utils::to_function(this->prev),
          args...);
      }
      
    private:
      functor_type target_;
    };
    
  private:
    functor_type target_;
  };
  
  template<typename functor>
  inline
  _task_step<decltype(&functor::operator())>
  task_step(const functor & f)
  {
    return _task_step<decltype(&functor::operator())>(f);
  }
  
  
}

#endif // __VDS_CORE_DATAFLOW_H_
