#ifndef __VDS_CORE_ASYNC_TASK_H_
#define __VDS_CORE_ASYNC_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "types.h"
#include <memory>
#include <atomic>
#include <list>

#include "func_utils.h"
#include "service_provider.h"

namespace vds {
  class service_provider;

  ////////////////////////////////////////////////////////////////
  template <typename functor_type, typename functor_signature>
  struct _async_task_functor_info;

  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  struct _async_task_functor_info<functor_type, result(class_name::*)(const service_provider & sp, arg_types...)>
  {
    typedef result signature(arg_types...);

    template<template<typename...> typename target_template>
    struct build_type
    {
      typedef target_template<arg_types...> type;
    };

    typedef std::tuple<arg_types...> arguments_typle;
    typedef std::function<signature> function_type;
    typedef result result_type;

    static std::function<signature> to_function(functor_type & f)
    {
      return std::function<signature>([&f](arg_types... args) { f(args...); });
    }
    static std::function<signature> to_function(functor_type && f)
    {
      return std::function<signature>(f);
    }
  };

  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  struct _async_task_functor_info<functor_type, result(class_name::*)(const service_provider & sp, arg_types...) const>
  {
    typedef result signature(arg_types...);
    typedef std::tuple<arg_types...> arguments_typle;
    typedef std::function<signature> function_type;
    typedef result result_type;

    template<template<typename...> typename target_template>
    struct build_type
    {
      typedef target_template<arg_types...> type;
    };

    static std::function<signature> to_function(functor_type & f)
    {
      return std::function<signature>([&f](arg_types ...args) { f(args...); });
    }

    static std::function<signature> to_function(functor_type && f)
    {
      return std::function<signature>(f);
    }
  };

  template <typename functor_type>
  struct async_task_functor_info : public _async_task_functor_info<functor_type, decltype(&functor_type::operator())>
  {};



  ////////////////////////////////////////////////////////////////
  template <typename function_signature>
  struct _async_task_arguments;

  template <typename done_method, typename class_name, typename... argument_types>
  struct _async_task_arguments<void (class_name::*)(const done_method &, const error_handler &, const service_provider & sp, argument_types...)>
  {
    typedef done_method done_method_type;
  };

  template <typename done_method, typename class_name, typename... argument_types>
  struct _async_task_arguments<void (class_name::*)(const done_method &, const error_handler &, const service_provider & sp, argument_types...) const>
  {
    typedef done_method done_method_type;
  };
  
  template <typename functor>
  struct async_task_arguments : public _async_task_arguments<decltype(&functor::operator())>
  {
  };
  ////////////////////////////////////////////////////////////////
  template <typename... arguments_types>
  class async_task
  {
  public:
    typedef void signature(arguments_types... args);

    async_task()
    {
    }
    
    async_task(const std::function<void(const std::function<void(const service_provider & sp, arguments_types... args)> &, const error_handler &, const service_provider &)> & target)
      : impl_(std::make_shared<_async_task>(target))
    {
    }

    template <typename functor>
    auto
      then(const functor & next_method, typename std::enable_if<!std::is_void<typename functor_info<functor>::result_type>::value>::type * = nullptr)
#ifndef _WIN32
      -> typename async_task_functor_info<functor>::result_type
#endif// _WIN32
      ;

    template <typename functor>
    auto
      then(const functor & next_method, typename std::enable_if<std::is_void<typename functor_info<functor>::result_type>::value>::type * = nullptr)
#ifndef _WIN32
      -> typename async_task_functor_info<typename async_task_arguments<functor>::done_method_type>::template build_type<async_task>::type
#endif// _WIN32
      ;

    void wait(
      const std::function<void(const service_provider & sp, arguments_types... args)> & done,
      const error_handler & on_error,
      const service_provider & sp) const
    {
      this->impl_->wait(done, on_error, sp);
    }
    
    async_task & operator = (const async_task & other)
    {
      this->impl_ = other.impl_;
      return *this;
    }

    static async_task<arguments_types...> empty()
    {
      return async_task([](
        const std::function<void(const service_provider & sp, arguments_types... args)> & done,
        const error_handler &,
        const service_provider & sp) {
        done(sp, arguments_types()...);
      });
    }

  private:

    class _async_task
    {
    public:
      _async_task(const std::function<void(
        const std::function<void(const service_provider & sp, arguments_types... args)> &,
        const error_handler &,
        const service_provider &)> & target)
        : target_(target)
#if _DEBUG
        , is_called_(false)
#endif
      {
      }

      ~_async_task()
      {
#if _DEBUG
        if (!this->is_called_) {
          throw std::runtime_error("Logic error 1");
        }
#endif
      }

      void wait(
        const std::function<void(const service_provider & sp, arguments_types... args)> & done,
        const error_handler & on_error,
        const service_provider & sp)
      {
#if _DEBUG
        if (this->is_called_) {
          throw std::runtime_error("Logic error 2");
        }
        this->is_called_ = true;
#endif
        try {
          this->target_(done, on_error, sp);
        }
        catch (const std::exception & ex) {
          on_error(sp, std::make_shared<std::runtime_error>(ex.what()));
        }
        catch (...) {
          on_error(sp, std::make_shared<std::runtime_error>("Unexpected error"));
        }
      }

    private:

#if _DEBUG
      bool is_called_;
#endif

      std::function<void(const std::function<void(const service_provider & sp, arguments_types... args)> &, const error_handler &, const service_provider &)> target_;
    };
    std::shared_ptr<_async_task> impl_;
  };

  template <typename functor>
  inline
    auto
    create_async_task(const functor & f) ->
    typename async_task_functor_info<typename _async_task_arguments<decltype(&functor::operator())>::done_method_type>::template build_type<async_task>::type
  {
    return typename async_task_functor_info<typename _async_task_arguments<decltype(&functor::operator())>::done_method_type>::template build_type<async_task>::type(f);
  }

  template<typename ...arguments_types>
  template<typename functor>
  inline auto async_task<arguments_types...>::then(
    const functor & next_method,
    typename std::enable_if<!std::is_void<typename functor_info<functor>::result_type>::value>::type *)
#ifndef _WIN32
    -> typename async_task_functor_info<functor>::result_type
#endif// _WIN32
  {
    using new_task_type = typename functor_info<functor>::result_type;
    auto p = this->impl_;
    return new_task_type([p, next_method](const std::function<typename add_first_parameter<const service_provider &, typename new_task_type::signature>::type> & done, const error_handler & on_error, const service_provider & sp)->void {
      p->wait(
        [next_method, done, on_error](const service_provider & sp, arguments_types... args) {
        try {
          next_method(sp, args...).wait(done, on_error, sp);
        }
        catch (const std::exception & ex) {
          on_error(sp, std::make_shared<std::exception>(ex));
        }
        catch (...) {
          on_error(sp, std::make_shared<std::runtime_error>("Unexpected error"));
        }
      }, on_error, sp);
    });
  }

  template<typename ...arguments_types>
  template<typename functor>
  inline auto async_task<arguments_types...>::then(
    const functor & next_method,
    typename std::enable_if<std::is_void<typename functor_info<functor>::result_type>::value>::type *)
#ifndef _WIN32
    -> typename async_task_functor_info<typename async_task_arguments<functor>::done_method_type>::template build_type<async_task>::type
#endif// _WIN32
  {
    auto p = this->impl_;
    return typename async_task_functor_info<typename async_task_arguments<functor>::done_method_type>::template build_type<::vds::async_task>::type(
      [p, next_method](
        const typename async_task_arguments<functor>::done_method_type & done,
        const error_handler & on_error,
        const service_provider & sp)->void {
      p->wait(
        [next_method, done, on_error](const vds::service_provider & sp, arguments_types... args) {
        try {
          next_method(done, on_error, sp, std::forward<arguments_types>(args)...);
        }
        catch (const std::exception & ex) {
          on_error(sp, std::make_shared<std::exception>(ex));
        }
        catch (...) {
          on_error(sp, std::make_shared<std::runtime_error>("Unexpected error"));
        }
      }, on_error, sp);
    });
  }
  
  class _async_series
  {
  public:
    _async_series(
      const std::function<void(const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp,
      size_t count)
    : done_(done), on_error_(on_error), sp_(sp), count_(count)
    {
    }
    
    void run(const std::list<async_task<>> & args)
    {
      for (auto arg : args) {
        *this += arg;
      }
    }
    
    _async_series & operator += (const async_task<> & arg)
    {
      arg.wait(
        [this](const service_provider & sp) {
          if (0 == --this->count_) {
            if (this->error_) {
              this->on_error_(sp, this->error_);
            }
            else {
              this->done_(sp);
            }
            delete this;
          }
        },
        [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          if (!this->error_) {
            this->error_ = ex;
          }
          if (0 == --this->count_) {
            if (this->error_) {
              this->on_error_(sp, this->error_);
            }
            else {
              this->done_(sp);
            }
            delete this;
          }
        },
        this->sp_);
      return *this;
    }
    
  private:
    std::function<void(const service_provider & sp)> done_;
    error_handler on_error_;
    service_provider sp_;
    std::atomic_size_t count_;
    std::shared_ptr<std::exception> error_;
  };
  
  template <typename... task_types>
  inline void _empty_fake(task_types... args)
  {
  }


  template <typename... task_types>
  inline async_task<> async_series(task_types... args)
  {
    auto steps = new std::list<async_task<>>({ args... });
    return create_async_task<>(
      [steps](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp){
        auto runner = new _async_series(done, on_error, sp, steps->size());
        runner->run(*steps);
        delete steps;
      });
  }
}

#endif // __VDS_CORE_ASYNC_TASK_H_
 