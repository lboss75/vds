#ifndef __VDS_CORE_DEFERRED_CALLBACK_H_
#define __VDS_CORE_DEFERRED_CALLBACK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <stack>
#include "func_utils.h"

namespace vds {
  class deferred_context;

  class _deferred_callback_base
  {
  public:
    _deferred_callback_base(
      deferred_context & context);

    virtual void execute() = 0;

  protected:
    deferred_context & context_;
    void schedule();
    bool direct_call_enabled();
  };

  class deferred_context : public std::enable_shared_from_this<deferred_context>
  {
  public:
    deferred_context();
    ~deferred_context();

    void operator()();

  private:
    friend class _deferred_callback_base;

    void schedule(_deferred_callback_base * callback);

    void continue_execute();

    std::mutex this_mutex_;
    bool in_execute_;
    std::stack<_deferred_callback_base *> next_;
  };

  template <typename functor_type, typename functor_signature>
  struct _deferred_callback;

  template <typename functor_type, typename class_name, typename... arg_types>
  class _deferred_callback<functor_type, void (class_name::*)(arg_types...)>
    : public _deferred_callback_base
  {
    using tuple_type = std::tuple<typename std::remove_reference<arg_types>::type...>;
  public:
    _deferred_callback(deferred_context & context, functor_type & target)
      : _deferred_callback_base(context), target_(target)
    {
    }

    void operator()(arg_types... args)
    {
      if (this->direct_call_enabled()) {
        this->target_(args...);
      }
      else {
        this->args_.reset(new tuple_type(args...));
        this->schedule();
      }
    }

    void execute() override
    {
      std::unique_ptr<tuple_type> args = std::move(this->args_);
      call_with<functor_type, tuple_type>(this->target_, *args);
    }

  private:
    functor_type & target_;
    std::unique_ptr<tuple_type> args_;
  };

  template <typename functor_type, typename class_name, typename... arg_types>
  class _deferred_callback<functor_type, void (class_name::*)(arg_types...) const>
    : public _deferred_callback_base
  {
    using tuple_type = std::tuple<typename std::remove_reference<arg_types>::type...>;
  public:
    _deferred_callback(deferred_context & context, functor_type & target)
      : _deferred_callback_base(context), target_(target)
    {
    }

    void operator()(arg_types... args)
    {
      if (this->direct_call_enabled()) {
        this->target_(args...);
      }
      else {
        this->args_.reset(new tuple_type(args...));
        this->schedule();
      }
    }

    void execute() override
    {
      std::unique_ptr<tuple_type> args = std::move(this->args_);
      call_with<functor_type, tuple_type>(this->target_, *args);
    }

  private:
    functor_type & target_;
    std::unique_ptr<tuple_type> args_;
  };

  template <typename functor_type>
  class deferred_callback
    : public _deferred_callback<functor_type, decltype(&functor_type::operator())>
  {
    using base_class = _deferred_callback<functor_type, decltype(&functor_type::operator())>;

  public:
    deferred_callback(deferred_context & context, functor_type & target)
      : base_class(context, target)
    {
    }
  };

}

#endif//__VDS_CORE_DEFERRED_CALLBACK_H_
