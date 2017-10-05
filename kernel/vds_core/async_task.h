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

namespace vds {
	template <typename... result_types>
	class async_task;

	template <typename... result_types>
	class async_result
	{
	public:
		async_result(
			std::function<void(result_types... results)> && done,
			std::function<void(const std::shared_ptr<std::exception> & ex)> && error);

		void operator()(result_types &&... results) const;
		void operator()(std::tuple<result_types...> && result) const;
		void error(const std::shared_ptr<std::exception> & ex) const;

	private:
		std::function<void(result_types... results)> done_;
		std::function<void(const std::shared_ptr<std::exception> & ex)> error_;
	};

  /////////////////////////////////////////////////////////////////////////////////
  template <typename argument_type>
  struct _is_async_task_result
  {
	  static constexpr bool value = false;
  };

  template <typename... result_types>
  struct _is_async_task_result<const async_result<result_types...> &>
  {
	static constexpr bool value = true;
    typedef async_task<result_types...> task_type;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename result_type>
  struct _async_task_tuple_result
  {
	  typedef async_task<result_type> task_type;
  };

  template <>
  struct _async_task_tuple_result<void>
  {
	  typedef async_task<> task_type;
  };

  template <typename... result_types>
  struct _async_task_tuple_result<std::tuple<result_types...>>
  {
	  typedef async_task<result_types...> task_type;
  };

  template <typename... result_types>
  struct _async_task_tuple_result<async_task<result_types...>>
  {
	  typedef async_task<result_types...> task_type;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template<typename functor_signature>
  struct _async_task_helper;

  template <typename result_type, typename class_name, typename... arg_types>
  struct _async_task_helper<result_type(class_name::*)(arg_types...)>
  {
	  typedef typename _async_task_tuple_result<result_type>::task_type task_type;
  };

  template <typename result_type, typename class_name, typename... arg_types>
  struct _async_task_helper<result_type(class_name::*)(arg_types...) const>
  {
	  typedef typename _async_task_tuple_result<result_type>::task_type task_type;
  };

  /////////////////////////////////////////////////////////////////////////////////
  template<typename functor_signature>
  struct _async_task_functor_helper
  {
	  static constexpr bool is_async_callback = false;
	  typedef typename _async_task_helper<functor_signature>::task_type task_type;
  };

  template <typename class_name, typename first_argument_type, typename... arg_types>
  struct _async_task_functor_helper<void(class_name::*)(first_argument_type, arg_types...) const>
  {
	  static constexpr bool is_async_callback = _is_async_task_result<first_argument_type>::value;
	  typedef typename std::conditional<
		  _is_async_task_result<first_argument_type>::value,
		  typename _is_async_task_result<first_argument_type>::task_type,
		  async_task<> >::type task_type;
  };

  template <typename class_name, typename first_argument_type, typename... arg_types>
  struct _async_task_functor_helper<void(class_name::*)(first_argument_type, arg_types...)>
  {
	  static constexpr bool is_async_callback = _is_async_task_result<first_argument_type>::value;
	  typedef typename std::conditional<
		  _is_async_task_result<first_argument_type>::value,
		  typename _is_async_task_result<first_argument_type>::task_type,
		  async_task<> >::type task_type;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename... result_types>
  class _async_task_base;
  /////////////////////////////////////////////////////////////////////////////////
  template <typename... result_types>
  class async_task
  {
  public:
	  template<typename functor_type>
	  async_task(functor_type && f,
		  typename std::enable_if<_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type * = nullptr);

	  template<typename functor_type>
	  async_task(
		  functor_type && f,
		  typename std::enable_if<!_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type * = nullptr);

	  async_task(async_task<result_types...> && origin);
	  ~async_task();

	  template<typename functor_type, typename error_functor_type>
	  void wait(functor_type && done_callback, error_functor_type && error_callback);

	  template<typename functor_type>
	  auto then(functor_type && f)
	  -> typename _async_task_functor_helper<decltype(&functor_type::operator())>::task_type;

	  template<typename functor_type, typename done_type>
	  void join(
		functor_type && f,
		done_type && done,
		typename std::enable_if<_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type * = nullptr);

	  template<typename functor_type, typename done_type>
	  void join(
		  functor_type && f,
		  done_type && done,
		  typename std::enable_if<!_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type * = nullptr);
  private:
	  _async_task_base<result_types...> * impl_;

	  template<typename parent_task_type, typename functor_type>
	  async_task(
		  parent_task_type && parent,
		  functor_type && f);
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename... result_types>
  class _async_task_base<result_types...>
  {
  public:
	  virtual ~_async_task_base() {}

	  virtual void execute(async_result<result_types...> && done) = 0;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename functor_type, typename... result_types>
  class _async_task_async_callback : public _async_task_base<result_types...>
  {
  public:
	  _async_task_async_callback(functor_type && f)
		  : f_(f)
	  {
	  }

	  void execute(async_result<result_types...> && done) override
	  {
		  this->f_(std::move(done));
	  }

  private:
	  functor_type f_;
  };

  template <typename functor_type, typename... result_types>
  class _async_task_sync_callback : public _async_task_base<result_types...>
  {
  public:
	  _async_task_sync_callback(functor_type && f)
		  : f_(f)
	  {
	  }

	  void execute(async_result<result_types...> && done) override
	  {
		  done(this->f_());
	  }

  private:
	  functor_type f_;
  };

  template <typename functor_type>
  class _async_task_sync_callback<functor_type> : public _async_task_base<>
  {
  public:
	  _async_task_sync_callback(functor_type && f)
		  : f_(f)
	  {
	  }

	  void execute(async_result<> && done) override
	  {
		  this->f_();
		  done();
	  }

  private:
	  functor_type f_;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename parent_task_type, typename functor_type, typename... result_types>
  class _async_task_joined_callback : public _async_task_base<result_types...>
  {
  public:
	  _async_task_joined_callback(parent_task_type && parent, functor_type && f)
		  : parent_(std::move(parent)), f_(std::move(f))
	  {
	  }

	  void execute(async_result<result_types...> && done) override
	  {
		  this->parent_.join(std::move(this->f_), std::move(done));
	  }

  private:
	  parent_task_type parent_;
	  functor_type f_;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template<typename ...result_types>
  template<typename functor_type>
  inline async_task<result_types...>::async_task(
	  functor_type && f,
	  typename std::enable_if<_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type *)
  : impl_(new _async_task_async_callback<functor_type, result_types...>(std::move(f)))
  {
  }
    
  template<typename ...result_types>
  template<typename functor_type>
  inline async_task<result_types...>::async_task(
	  functor_type && f,
	  typename std::enable_if<!_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type *)
  : impl_(new _async_task_sync_callback<functor_type, result_types...>(std::move(f)))
  {
  }

  template<typename ...result_types>
  inline async_task<result_types...>::async_task(async_task<result_types...>&& origin)
  : impl_(origin.impl_)
  {
	  origin.impl_ = nullptr;
  }

  template<typename ...result_types>
  template<typename parent_task_type, typename functor_type>
  inline async_task<result_types...>::async_task(
	  parent_task_type && parent,
	  functor_type && f)
  : impl_(new _async_task_joined_callback<parent_task_type, functor_type, result_types...>(std::move(parent), std::move(f)))
  {
  }

  template<typename ...result_types>
  inline async_task<result_types...>::~async_task()
  {
	  delete this->impl_;
  }

  template<typename ...result_types>
  template<typename functor_type, typename error_functor_type>
  inline void async_task<result_types...>::wait(functor_type && done_callback, error_functor_type && error_callback)
  {
	  this->impl_->execute(
		  async_result<result_types...>(
			  std::function<void(result_types...)>(std::move(done_callback)),
			  std::function<void(const std::shared_ptr<std::exception> &)>(std::move(error_callback))));
  }

  template<typename ...result_types>
  template<typename functor_type>
  inline auto async_task<result_types...>::then(functor_type && f)
	  -> typename _async_task_functor_helper<decltype(&functor_type::operator())>::task_type
  {
	  return _async_task_functor_helper<decltype(&functor_type::operator())>::task_type(
		  std::move(*this),
		  std::move(f));
  }

  template<typename ...result_types>
  template<typename functor_type, typename done_type>
  inline void async_task<result_types...>::join(
	functor_type && f,
	done_type && done,
	typename std::enable_if<_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type *)
  {
	  this->impl_->execute(
		  async_result<result_types...>(
			  std::function<void(result_types...)>([f_ = std::move(f), done](result_types... result) {
				f_(done, std::forward<result_types>(result)...);
			}),
			  std::function<void(const std::shared_ptr<std::exception> &)>([done](const std::shared_ptr<std::exception> & ex) {
				done.error(ex);
			})));
  }

  template<typename ...result_types>
  template<typename functor_type, typename done_type>
  inline void async_task<result_types...>::join(
	  functor_type && f,
	  done_type && done,
	  typename std::enable_if<!_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type *)
  {
	  this->impl_->execute(
		  async_result<result_types...>(
			  std::function<void(result_types...)>([f_ = std::move(f), done](result_types... result) {
		  f_(std::forward<result_types>(result)...);
	  }),
			  std::function<void(const std::shared_ptr<std::exception> &)>([done](const std::shared_ptr<std::exception> & ex) {
		  done.error(ex);
	  })));
  }

  /////////////////////////////////////////////////////////////////////////////////
  template<typename ...result_types>
  inline async_result<result_types...>::async_result(
	  std::function<void(result_types...results)> && done,
	  std::function<void(const std::shared_ptr<std::exception>&ex)> && error)
	  : done_(done), error_(error)
  {
  }

  template<typename ...result_types>
  inline void async_result<result_types...>::operator()(result_types && ...results) const
  {
	  this->done_(std::forward<result_types>(results)...);
  }

  template<typename ...result_types>
  inline void async_result<result_types...>::operator()(std::tuple<result_types...> && result) const
  {
	  call_with(this->done_, std::move(result));
  }

  /////////////////////////////////////////////////////////////////////////////////
  /*
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
  }*/
}

#endif // __VDS_CORE_ASYNC_TASK_H_
 
