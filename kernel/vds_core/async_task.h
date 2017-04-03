#ifndef __VDS_CORE_ASYNC_TASK_H_
#define __VDS_CORE_ASYNC_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "types.h"
#include <memory>
#include "func_utils.h"

namespace vds {
  ////////////////////////////////////////////////////////////////
  template <typename function_signature>
  struct _async_task_arguments;

  template <typename done_method, typename class_name, typename... argument_types>
  struct _async_task_arguments<void (class_name::*)(const done_method &, const error_handler &, argument_types...)>
  {
    typedef done_method done_method_type;
  };

  template <typename done_method, typename class_name, typename... argument_types>
  struct _async_task_arguments<void (class_name::*)(const done_method &, const error_handler &, argument_types...) const>
  {
    typedef done_method done_method_type;
  };
  ////////////////////////////////////////////////////////////////
  template <typename signature>
  class async_task;

  template <typename... arguments_types>
  class async_task<void(arguments_types... args)>
  {
  public:
    typedef void signature(arguments_types... args);

    async_task(const std::function<void(const std::function<void(arguments_types... args)> &, const error_handler &)> & target)
      : impl_(std::make_shared<_async_task>(target))
    {
    }

    template <typename functor>
    auto
    then(const functor & next_method, typename std::enable_if<!std::is_void<typename functor_info<functor>::result_type>::value>::type * = nullptr)
      -> typename functor_info<functor>::result_type
    {
      using new_task_type = typename functor_info<functor>::result_type;
      auto p = this->impl_;
      return new_task_type([p, next_method](const std::function<typename new_task_type::signature> & done, const error_handler & on_error)->void {
        p->wait([next_method, done, on_error](arguments_types... args) {
          next_method(args...).wait(done, on_error);
        }, on_error);
      });
    }

    template <typename functor>
    auto
      then(const functor & next_method, typename std::enable_if<std::is_void<typename functor_info<functor>::result_type>::value>::type * = nullptr)
      -> async_task<typename functor_info<typename _async_task_arguments<functor>::done_method_type>::signature>
    {
      auto p = this->impl_;
      return async_task<typename functor_info<typename _async_task_arguments<functor>::done_method_type>::signature>(
        [p, next_method](const typename _async_task_arguments<functor>::done_method_type & done, const error_handler & on_error)->void {
        p->wait([next_method, done, on_error](arguments_types... args) {
          next_method(done, on_error, args...);
        }, on_error);
      });
    }

    void wait(const std::function<void(arguments_types... args)> & done, const error_handler & on_error)
    {
      this->impl_->wait(done, on_error);
    }

  private:

    class _async_task
    {
    public:
      _async_task(const std::function<void(const std::function<void(arguments_types... args)> &, const error_handler &)> & target)
        : target_(target)
      {
      }

      void wait(const std::function<void(arguments_types... args)> & done, const error_handler & on_error)
      {
        this->target_(done, on_error);
      }

    private:
      std::function<void(const std::function<void(arguments_types... args)> &, const error_handler &)> target_;
    };
    std::shared_ptr<_async_task> impl_;
  };

  template <typename functor>
  inline
    auto
    create_async_task(const functor & f) ->
    async_task<typename functor_info<typename _async_task_arguments<decltype(&functor::operator())>::done_method_type>::signature>
  {
    return async_task<typename functor_info<typename _async_task_arguments<decltype(&functor::operator())>::done_method_type>::signature>(f);
  }
}

#endif // __VDS_CORE_ASYNC_TASK_H_
 