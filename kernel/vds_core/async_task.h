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
#include "result_or_error.h"

namespace vds {
	template <typename... result_types>
	class async_task;

	template <typename... result_types>
	class async_result
	{
	public:
		async_result(std::function<void(const std::shared_ptr<std::exception> & ex, result_types... results)> && done);

		void done(const result_types &... results) const;
		void done(std::tuple<result_types...> && result) const;
		void error(const std::shared_ptr<std::exception> & ex) const;

	private:
    
    std::function<void(const std::shared_ptr<std::exception> & ex, result_types... results)> done_;
	};

  /////////////////////////////////////////////////////////////////////////////////
  template <typename argument_type>
  struct _is_async_task_result
  {
	  static constexpr bool value = false;
    static constexpr bool is_void_result = true;
    typedef void task_type;
  };

  template <typename... result_types>
  struct _is_async_task_result<const async_result<result_types...> &>
  {
    static constexpr bool value = true;
    static constexpr bool is_void_result = (0 == sizeof...(result_types));
    typedef async_task<result_types...> task_type;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename result_type>
  struct _async_task_result_helper
  {
	  typedef async_task<result_type> task_type;
    static constexpr bool task_as_result = false;
  };

  template <>
  struct _async_task_result_helper<void>
  {
	  typedef async_task<> task_type;
    static constexpr bool task_as_result = false;
  };

  template <typename... result_types>
  struct _async_task_result_helper<std::tuple<result_types...>>
  {
	  typedef async_task<result_types...> task_type;
    static constexpr bool task_as_result = false;
  };

  template <typename... result_types>
  struct _async_task_result_helper<async_task<result_types...>>
  {
	  typedef async_task<result_types...> task_type;
    static constexpr bool task_as_result = true;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template<typename functor_signature>
  struct _async_task_helper;

  template <typename result_type, typename class_name, typename... arg_types>
  struct _async_task_helper<result_type(class_name::*)(arg_types...)>
  {
	  typedef typename _async_task_result_helper<result_type>::task_type task_type;
	  static constexpr bool task_as_result = _async_task_result_helper<result_type>::task_as_result;
	  static constexpr bool is_void_result = std::is_void<result_type>::value;
  };

  template <typename result_type, typename class_name, typename... arg_types>
  struct _async_task_helper<result_type(class_name::*)(arg_types...) const>
  {
	  typedef typename _async_task_result_helper<result_type>::task_type task_type;
	  static constexpr bool task_as_result = _async_task_result_helper<result_type>::task_as_result;
	  static constexpr bool is_void_result = std::is_void<result_type>::value;
  };

  /////////////////////////////////////////////////////////////////////////////////
  template<typename functor_signature>
  struct _async_task_functor_helper
  {
	  static constexpr bool is_async_callback = false;
	  static constexpr bool task_as_result = _async_task_helper<functor_signature>::task_as_result;
	  static constexpr bool is_void_result = _async_task_helper<functor_signature>::is_void_result;
	  typedef typename _async_task_helper<functor_signature>::task_type task_type;
  };

  template <typename class_name, typename first_argument_type, typename... arg_types>
  struct _async_task_functor_helper<void(class_name::*)(first_argument_type, arg_types...) const>
  {
	  static constexpr bool is_async_callback = _is_async_task_result<first_argument_type>::value;
	  static constexpr bool task_as_result = false;
    static constexpr bool is_void_result = 
      !_is_async_task_result<first_argument_type>::value
      || _is_async_task_result<first_argument_type>::is_void_result;

	  typedef typename std::conditional<
		  _is_async_task_result<first_argument_type>::value,
		  typename _is_async_task_result<first_argument_type>::task_type,
		  async_task<> >::type task_type;
  };

  template <typename class_name, typename first_argument_type, typename... arg_types>
  struct _async_task_functor_helper<void(class_name::*)(first_argument_type, arg_types...)>
  {
	  static constexpr bool is_async_callback = _is_async_task_result<first_argument_type>::value;
	  static constexpr bool task_as_result = false;
    static constexpr bool is_void_result =
      !_is_async_task_result<first_argument_type>::value
      || _is_async_task_result<first_argument_type>::is_void_result;
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
	  async_task(std::shared_ptr<std::exception> && error);
	  async_task(const std::shared_ptr<std::exception> & error);
    
	  template<typename parent_task_type, typename functor_type>
	  async_task(
		  parent_task_type && parent,
		  functor_type && f);
    
	  ~async_task();

	  template<typename functor_type>
	  void execute(functor_type && done_callback);
    
	  template<typename functor_type>
    void operator()(functor_type && done_callback);

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
		  typename std::enable_if<
		  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
		  && _async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result>::type * = nullptr);
    
	  template<typename functor_type, typename done_type>
	  void join(
		  functor_type && f,
		  done_type && done,
		  typename std::enable_if<
		  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
		  && !_async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result
		  && _async_task_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type * = nullptr);
    
	  template<typename functor_type, typename done_type>
	  void join(
		  functor_type && f,
		  done_type && done,
		  typename std::enable_if<
		  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
		  && !_async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result
		  && !_async_task_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type * = nullptr);
    
    static async_task<result_types...> empty();
  private:
	  _async_task_base<result_types...> * impl_;
    
	  async_task(_async_task_base<result_types...> * impl);
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename... result_types>
  class _async_task_base
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
		  : f_(std::move(f))
	  {
	  }

	  void execute(async_result<result_types...> && done) override
	  {
      try {
        this->f_(done);
      }
      catch (const std::exception & ex) {
        done.error(std::make_shared<std::runtime_error>(ex.what()));
      }
      catch (...) {
        done.error(std::make_shared<std::runtime_error>("Unexpected error"));
      }
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
      try {
		    done.done(this->f_());
      }
      catch (const std::exception & ex) {
        done.error(std::make_shared<std::runtime_error>(ex.what()));
      }
      catch (...) {
        done.error(std::make_shared<std::runtime_error>("Unexpected error"));
      }

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
      try {
        this->f_();
      }
      catch (const std::exception & ex) {
        done.error(std::make_shared<std::runtime_error>(ex.what()));
        return;
      }
      catch (...) {
        done.error(std::make_shared<std::runtime_error>("Unexpected error"));
        return;
      }

      done.done();
	  }

  private:
	  functor_type f_;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename... result_types>
  class _async_task_error : public _async_task_base<result_types...>
  {
  public:
	  _async_task_error(std::shared_ptr<std::exception> && error)
		  : error_(std::move(error))
	  {
	  }
	  
	  _async_task_error(const std::shared_ptr<std::exception> & error)
		  : error_(error)
	  {
	  }

	  void execute(async_result<result_types...> && done) override
	  {
      done.error(this->error_);
	  }

  private:
	  std::shared_ptr<std::exception> error_;
  };
  /////////////////////////////////////////////////////////////////////////////////
  template <typename... result_types>
  class _async_task_empty : public _async_task_base<result_types...>
  {
  public:
	  _async_task_empty()
	  {
	  }

	  void execute(async_result<result_types...> && done) override
	  {
      done(result_types()...);
	  }
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
  inline async_task<result_types...>::async_task(std::shared_ptr<std::exception> && error)
  : impl_(new _async_task_error<result_types...>(std::move(error)))
  {
  }
  	  
  template<typename ...result_types>
  inline async_task<result_types...>::async_task(const std::shared_ptr<std::exception> & error)
  : impl_(new _async_task_error<result_types...>(error))
  {
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
  inline async_task<result_types...>::async_task(_async_task_base<result_types...> * impl)
  : impl_(impl)
  {
  }

  template<typename ...result_types>
  inline async_task<result_types...>::~async_task()
  {
    if(nullptr != this->impl_){
      throw std::runtime_error("Task without execute");
    }
    
	  delete this->impl_;
  }

  template<typename ...result_types>
  template<typename functor_type>
  inline void async_task<result_types...>::execute(functor_type && done_callback)
  {
    auto impl = this->impl_;
    this->impl_ = nullptr;
    impl->execute(
		  async_result<result_types...>(
			  std::move(done_callback)));
  }
  
  template<typename ...result_types>
  template<typename functor_type>
  inline void async_task<result_types...>::operator()(
    functor_type && done_callback)
  {
	  this->impl_->execute(
      async_result<result_types...>(
        [done = std::move(done_callback)](const std::shared_ptr<std::exception> & ex, result_types... results){
          if(!ex){
            done.done(std::forward<result_types>(results)...);
          } else {
            done.error(ex);
          }
        }));
  }

  template<typename ...result_types>
  template<typename functor_type>
  inline auto async_task<result_types...>::then(functor_type && f)
	  -> typename _async_task_functor_helper<decltype(&functor_type::operator())>::task_type
  {
	  return typename _async_task_functor_helper<decltype(&functor_type::operator())>::task_type(
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
			  std::function<void(result_types...)>([f_ = std::move(f), d = std::move(done)](const std::shared_ptr<std::exception> & ex, result_types... result) {
          if(!ex){
            f_(d, std::forward<result_types>(result)...);
          } else {
            d.error(ex);
          }
			})));
  }

  template<typename ...result_types>
  template<typename functor_type, typename done_type>
  inline void async_task<result_types...>::join(
	  functor_type && f,
	  done_type && done,
	  typename std::enable_if<
	  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
	  && _async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result>::type *)
  {
	  this->impl_->execute(
		  async_result<result_types...>([f_ = std::move(f), d = std::move(done)](const std::shared_ptr<std::exception> & ex, result_types... result) {
        if(!ex){
          auto t = f_(std::forward<result_types>(result)...);
          t(std::move(d));
        } else {
          d.error(ex);
        }
	  }));
  }

  template<typename ...result_types>
  template<typename functor_type, typename done_type>
  inline void async_task<result_types...>::join(
	  functor_type && f,
	  done_type && done,
	  typename std::enable_if<
	  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
	  && !_async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result
	  && _async_task_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type *)
  {
	  this->impl_->execute(
		  async_result<result_types...>(
			  [f_ = std::move(f), done](const std::shared_ptr<std::exception> & ex, result_types... result) {
          if(!ex){
            try {
              f_(std::forward<result_types>(result)...);
            } catch(const std::exception & ex){
              done.error(std::make_shared<std::runtime_error>(ex.what()));
              return;
            }        
            done.done();
          } else {
            done.error(ex);
          }
	  }));
  }
  
  template<typename ...result_types>
  template<typename functor_type, typename done_type>
  inline void async_task<result_types...>::join(
	  functor_type && f,
	  done_type && done,
	  typename std::enable_if<
	  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
	  && !_async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result
	  && !_async_task_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type *)
  {
	  this->impl_->execute(async_result<result_types...>([f_ = std::move(f), done](
          const std::shared_ptr<std::exception> & ex,
          result_types... result) {
          if(!ex){
            try {
              auto t = f_(std::forward<result_types>(result)...);
              done.done(t);
            } catch(const std::exception & ex){
              done.error(std::make_shared<std::runtime_error>(ex.what()));
            }        
          } else {
            done.error(ex);
          }
        }));
  }
  
  template<typename ...result_types>
  inline async_task<result_types...> async_task<result_types...>::empty()
  {
    return async_task<result_types...>(new _async_task_empty<result_types...>());
  }
  /////////////////////////////////////////////////////////////////////////////////
  template<typename ...result_types>
  inline async_result<result_types...>::async_result(
	  std::function<void(const std::shared_ptr<std::exception> & ex, result_types...results)> && done)
	: done_(std::move(done))
  {
  }

  template<typename ...result_types>
  inline void async_result<result_types...>::done(const result_types & ...results) const
  {
	  this->done_(std::shared_ptr<std::exception>(), results...);
  }

  template<typename ...result_types>
  inline void async_result<result_types...>::done(std::tuple<result_types...> && result) const
  {
	  call_with(this->done_, std::shared_ptr<std::exception>(), std::move(result));
  }

  template<typename ...result_types>
  inline void async_result<result_types...>::error(const std::shared_ptr<std::exception>& ex) const
  {
	  this->done_(ex, result_types()...);
  }

  /////////////////////////////////////////////////////////////////////////////////
  class _async_series
  {
  public:
    _async_series(
      const async_result<> & result,
      size_t count)
    : result_(result), count_(count)
    {
    }
    
    void run(std::list<async_task<>> && args)
    {
      for (auto && arg : args) {
        this->add(std::move(arg));
      }
    }
    
    _async_series & add(async_task<> && arg)
    {
      arg.execute(
        [this](const std::shared_ptr<std::exception> & ex) {
          if(ex && !this->error_) {
              this->error_ = ex;
          }
          
          if (0 == --this->count_) {
            if (this->error_) {
              this->result_.error(this->error_);
            }
            else {
              this->result_.done();
            }            
            delete this;
          }
        });
      return *this;
    }
    
  private:
    async_result<> result_;
    std::atomic_size_t count_;
    std::shared_ptr<std::exception> error_;
  };
  
  template <typename... task_types>
  inline void _empty_fake(task_types... args)
  {
  }
  
  template <typename task_type>
  inline void _add_to_async_series(std::list<async_task<>> & target, task_type && first)
  {
    target.push_back(std::move(first));
  }
  
  template <typename task_type, typename... task_types>
  inline void _add_to_async_series(std::list<async_task<>> & target, task_type && first, task_types &&... args)
  {
    target.push_back(std::move(first));
    _add_to_async_series(target, std::move(args)...);
  }


  template <typename... task_types>
  inline async_task<> async_series(task_types &&... args)
  {
    auto steps = new std::list<async_task<>>();
    _add_to_async_series(*steps, std::move(args)...);
    
    return [steps](const async_result<> & result){
        auto runner = new _async_series(result, steps->size());
        runner->run(std::move(*steps));
        delete steps;
      };
  }
}

#endif // __VDS_CORE_ASYNC_TASK_H_
 
