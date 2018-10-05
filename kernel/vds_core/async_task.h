#ifndef __VDS_CORE_ASYNC_TASK_H_
#define __VDS_CORE_ASYNC_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <future>

#ifndef _WIN32

#include <experimental/coroutine>
#include "vds_debug.h"

namespace std {
  namespace experimental {
    template<typename R, typename... Args>
    struct coroutine_traits<std::future<R>, Args...> {
      struct promise_type {
        std::promise<R> p;

        auto get_return_object() {
          return p.get_future();
        }

        std::experimental::suspend_never initial_suspend() {
          return {};
        }

        std::experimental::suspend_never final_suspend() {
          return {};
        }

        void set_exception(std::exception_ptr e) {
          p.set_exception(std::move(e));
        }

        template<typename U>
        void return_value(U &&u) {
          p.set_value(std::forward<U>(u));
        }

        void unhandled_exception() {
          p.set_exception(std::current_exception());
        }
      };
    };
    template<typename... Args>
    struct coroutine_traits<std::future<void>, Args...> {
      struct promise_type {
        std::promise<void> p;

        auto get_return_object() {
          return p.get_future();
        }

        std::experimental::suspend_never initial_suspend() {
          return {};
        }

        std::experimental::suspend_never final_suspend() {
          return {};
        }

        void set_exception(std::exception_ptr e) {
          p.set_exception(std::move(e));
        }

        void return_void() {
          p.set_value();
        }

        void unhandled_exception() {
          p.set_exception(std::current_exception());
        }
      };
    };
  };
}

namespace vds {
  template<typename T>
  struct awaiter {
    std::future<T> _future;
  public:
    explicit awaiter(std::future<T> &&f) noexcept : _future(std::move(f)) {
    }

    bool await_ready() const noexcept {
      return _future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    template<typename U>
    void await_suspend(std::experimental::coroutine_handle<U> hndl) noexcept {
      std::async([this, hndl] () mutable {
        this->_future.wait();
        hndl.resume();
      });
    }

    T await_resume() {
      return _future.get(); }
  };

  template<>
  struct awaiter<void> {
    std::future<void> _future;
  public:
    explicit awaiter(std::future<void> &&f) noexcept : _future(std::move(f)) {}

    bool await_ready() const noexcept {
      return _future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

<<<<<<< HEAD
    while(token->process());
  }
  
  template<typename ...result_types>
  template<typename functor_type>
  inline void async_task<result_types...>::operator()(
		const std::shared_ptr<_async_task_execute_token> & token,
	  functor_type && done_callback)
  {
		auto impl = this->impl_;
		this->impl_ = nullptr;
    done_callback.add_owner(impl);
	  impl->execute(token, std::move(done_callback));
  }

  template<typename ...result_types>
  template<typename functor_type>
  inline auto async_task<result_types...>::then(functor_type && f)
	  -> typename _async_task_functor_helper<decltype(&functor_type::operator())>::task_type
  {
	  return typename _async_task_functor_helper<decltype(&functor_type::operator())>::task_type(
		  std::move(*this),
      std::forward<functor_type>(f));
  }

  template<typename ...result_types>
  template<typename functor_type, typename done_type>
  inline void async_task<result_types...>::execute_with(
  const std::shared_ptr<_async_task_execute_token> & token,
	functor_type & f,
	done_type && done,
	typename std::enable_if<_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type *)
  {
		auto impl = this->impl_;
		this->impl_ = nullptr;
	  impl->execute(
		  async_result<result_types...>(
        impl,
			  [&f, d = std::move(done), token](const std::shared_ptr<std::exception> & ex, result_types... result) {
          if(!ex){
            f(d, std::forward<result_types>(result)...);
          } else {
            d.error(ex);
          }
			}));
  }

  template<typename ...result_types>
  template<typename functor_type, typename done_type>
  inline void async_task<result_types...>::execute_with(
    const std::shared_ptr<_async_task_execute_token> & token,
	  functor_type & f,
	  done_type && done,
	  typename std::enable_if<
	  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
	  && _async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result>::type *)
  {
		auto impl = this->impl_;
		this->impl_ = nullptr;
	  impl->execute(
			token,
		  async_result<result_types...>(token, impl, [token, &f, d = std::move(done)](const std::shared_ptr<std::exception> & ex, result_types... result) mutable {
        if(!ex){
			try {
				auto t = f(std::forward<result_types>(result)...);
#ifndef _WIN32 
        t.template operator()<done_type>(token, std::move(d));
#else
        t.operator()<done_type>(token, std::move(d));
#endif
			}
			catch (const std::exception & ex) {
				d.error(std::make_shared<std::runtime_error>(ex.what()));
			}
        } else {
          d.error(ex);
        }
	  }));
  }

  template<typename ...result_types>
  template<typename functor_type, typename done_type>
  inline void async_task<result_types...>::execute_with(
    const std::shared_ptr<_async_task_execute_token> & token,
	  functor_type & f,
	  done_type && done,
	  typename std::enable_if<
	  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
	  && !_async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result
	  && _async_task_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type *)
  {
		auto impl = this->impl_;
		this->impl_ = nullptr;
	  impl->execute(
			token,
		  async_result<result_types...>(
        token,
        impl,
			  [token, &f, done](const std::shared_ptr<std::exception> & ex, result_types... result) {
          if(!ex){
            try {
              f(std::forward<result_types>(result)...);
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
  inline void async_task<result_types...>::execute_with(
    const std::shared_ptr<_async_task_execute_token> & token,
	  functor_type & f,
	  done_type && done,
	  typename std::enable_if<
	  !_async_task_functor_helper<decltype(&functor_type::operator())>::is_async_callback
	  && !_async_task_functor_helper<decltype(&functor_type::operator())>::task_as_result
	  && !_async_task_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type *)
  {
		auto impl = this->impl_;
		this->impl_ = nullptr;
	  impl->execute(token, async_result<result_types...>(token, impl, [&f, done, token](
          const std::shared_ptr<std::exception> & ex,
          result_types... result) {
          if(!ex){
            try {
              auto t = f(std::forward<result_types>(result)...);
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
    return async_task<result_types...>(static_cast<_async_task_base<result_types...> *>(new _async_task_empty<result_types...>()));
  }

	template<typename ...result_types>
	inline async_task<result_types...> &async_task<result_types...>::operator=(async_task<result_types...> &&origin) {
		delete this->impl_;
		this->impl_ = origin.impl_;
		origin.impl_ = nullptr;
		return *this;
	}

	/////////////////////////////////////////////////////////////////////////////////
  template<typename ...result_types>
  inline async_result<result_types...>::async_result(
    const std::shared_ptr<_async_task_execute_token> & token,
    _async_task_base<result_types...> * owner,
	  std::function<void(const std::shared_ptr<std::exception> & ex, result_types...results)> && callback)
	: impl_(new result_callback(token, owner, std::move(callback)))
  {
  }

  template<typename ...result_types>
  inline void async_result<result_types...>::done(const result_types & ...results) const
  {
    auto impl = std::move(this->impl_);
    impl->token_->set_callback([impl, results...](){
      impl->done_ = true;
      impl->callback_(std::shared_ptr<std::exception>(), results...);
    });
  }

  template<typename ...result_types>
  inline void async_result<result_types...>::error(const std::shared_ptr<std::exception>& ex) const
  {
    auto impl = std::move(this->impl_);
    impl->token_->set_callback([impl, ex]() {
      impl->done_ = true;
      impl->callback_(ex, typename std::remove_reference<result_types>::type()...);
    });
  }

  template<typename ...result_types>
  inline void async_result<result_types...>::clear() {
		this->impl_.reset();
  }

  template <typename ... result_types>
  void async_result<result_types...>::add_owner(_async_task_base<result_types...>* owner) {
    this->impl_->add_owner(owner);
  }

  template <typename ... result_types>
  async_result<result_types...>::result_callback::~result_callback() {
    for(auto owner : this->owners_) {
      delete owner;
=======
    template<typename U>
    void await_suspend(std::experimental::coroutine_handle<U> hndl) noexcept {
      std::async([this, hndl]() mutable {
        this->_future.wait();
        hndl.resume();
      });
>>>>>>> coroutines
    }

    void await_resume() { _future.get(); }
  };
}

namespace std {
  template<typename T>
  inline auto operator co_await(std::future<T> f) noexcept {
    return vds::awaiter<T>(std::move(f));
  }

  inline auto operator co_await(std::future<void> f) noexcept {
    return vds::awaiter<void>(std::move(f));
  }
}
#endif//_WIN32

#endif // __VDS_CORE_ASYNC_TASK_H_
 
