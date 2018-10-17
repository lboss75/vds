#ifndef __VDS_CORE_ASYNC_TASK_H_
#define __VDS_CORE_ASYNC_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <experimental/coroutine>
#include <future>
#include <chrono>
#include <functional>
#include "vds_debug.h"

namespace vds {
  template <typename result_type>
  class async_task;

  template <typename result_type>
  class async_result;

  template <typename result_type>
  class _async_task_value {
  public:
    virtual ~_async_task_value(){}
    virtual result_type & get() = 0;
  };

  template <>
  class _async_task_value<void> {
  public:
    virtual ~_async_task_value() {}
    virtual void get() = 0;
  };

  template <typename result_type>
  class _async_task_return_value : public _async_task_value<result_type>{
  public:

    template<typename init_type>
    _async_task_return_value(init_type && v)
    : value_(std::forward<init_type>(v)) {
    }

    result_type & get() override {
      return this->value_;
    }

  private:
    result_type value_;
  };

  template <>
  class _async_task_return_value<void> : public _async_task_value<void> {
  public:

    void get() override {
    }
  };

  template <typename result_type>
  class _async_task_throw_exception : public _async_task_value<result_type> {
  public:
    _async_task_throw_exception(std::exception_ptr ex)
      : ex_(ex) {
    }

    result_type & get() override {
      std::rethrow_exception(this->ex_);
    }

  private:
    std::exception_ptr ex_;
  };

  template <>
  class _async_task_throw_exception<void> : public _async_task_value<void> {
  public:
    _async_task_throw_exception(std::exception_ptr ex)
      : ex_(ex) {
    }

    void get() override {
      std::rethrow_exception(this->ex_);
    }

  private:
    std::exception_ptr ex_;
  };

  template <typename result_type>
  class _async_task_state {
  public:
    _async_task_state()
    : is_processed_(false) {
    }

#ifdef DEBUG
    ~_async_task_state() noexcept(false) {
      vds_assert(this->is_processed_);
    }
#endif

    template<class _Rep, class _Period>
    std::future_status wait_for(std::chrono::duration<_Rep, _Period> timeout) {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      if (this->value_) {
        return std::future_status::ready;
      }

      this->value_cond_.wait_for(lock, timeout);
      return (!this->value_) ? std::future_status::timeout : std::future_status::ready;
    }

    bool is_ready() {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      return (!this->value_) ? false : true;
    }


    auto & get() {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      while (!this->value_) {
        this->value_cond_.wait(lock);
      }
      vds_assert(!this->is_processed_);
      this->is_processed_ = true;
      return this->value_->get();
    }

    void then(const std::function<void(void)> & f) {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      if (!this->value_) {
        this->then_function_ = f;
      }
      else {
        lock.unlock();

        f();
      }
    }

    void set_value(_async_task_value<result_type> * v) {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      vds_assert(!this->value_);
      this->value_.reset(v);
      this->value_cond_.notify_all();
      if(this->then_function_) {
        lock.unlock();

        this->then_function_();
      }
    }

    bool is_processed() const {
      return this->is_processed_;
    }
  private:
    std::mutex value_mutex_;
    std::condition_variable value_cond_;
    bool is_processed_;
    std::unique_ptr<_async_task_value<result_type>> value_;
    std::function<void(void)> then_function_;
  };

  template <>
  class _async_task_state<void> {
  public:
    _async_task_state()
      : is_processed_(false) {
    }

#ifdef DEBUG
    ~_async_task_state() noexcept(false) {
      vds_assert(this->is_processed_);
    }
#endif

    template<class _Rep, class _Period>
    std::future_status wait_for(std::chrono::duration<_Rep, _Period> timeout) {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      if (this->value_) {
        return std::future_status::ready;
      }

      this->value_cond_.wait_for(lock, timeout);
      return (!this->value_) ? std::future_status::timeout : std::future_status::ready;
    }

    bool is_ready() {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      return (!this->value_) ? false : true;
    }


    void get() {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      while (!this->value_) {
        this->value_cond_.wait(lock);
      }
      vds_assert(!this->is_processed_);
      this->is_processed_ = true;
      this->value_->get();
    }

    void then(const std::function<void(void)> & f) {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      if (!this->value_) {
        this->then_function_ = f;
      }
      else {
        lock.unlock();

        f();
      }
    }

    void set_value(_async_task_value<void> * v) {
      std::unique_lock<std::mutex> lock(this->value_mutex_);
      vds_assert(!this->value_);
      this->value_.reset(v);
      this->value_cond_.notify_all();
      if (this->then_function_) {
        lock.unlock();

        this->then_function_();
      }
    }

    bool is_processed() const {
      return this->is_processed_;
    }
  private:
    std::mutex value_mutex_;
    std::condition_variable value_cond_;
    bool is_processed_;
    std::unique_ptr<_async_task_value<void>> value_;
    std::function<void(void)> then_function_;
  };

  template <typename result_type>
  class async_task {
  public:
    async_task() = delete;
    async_task(const async_task &) = delete;
    async_task(async_task &&) = default;

    async_task & operator = (const async_task &) = delete;
    async_task & operator = (async_task &&) = default;

    async_task(const std::shared_ptr<_async_task_state<result_type>> & state)
    : state_(state) {      
    }

#ifdef DEBUG
    ~async_task() noexcept(false) {
      vds_assert(!this->state_ || this->state_->is_processed());
    }
#endif

    template<class _Rep, class _Period>
    std::future_status wait_for(std::chrono::duration<_Rep, _Period> timeout) const {
      return this->state_->wait_for(timeout);
    }

    auto & get() {
      return this->state_->get();
    }

    bool is_ready() const noexcept {
      return this->state_->is_ready();
    }

    void then(const std::function<void(void)> & f) {
      this->state_->then(f);
    }

    void detach() {
      auto s = std::move(this->state_);
      s->then([s]() {
        try {
          s->get();
        }
        catch(...) {          
        }
      });
    }
  private:
    std::shared_ptr<_async_task_state<result_type>> state_;
  };  

  template <>
  class async_task<void> {
  public:
    async_task() = delete;
    async_task(const async_task &) = delete;
    async_task(async_task &&) = default;

    async_task & operator = (const async_task &) = delete;
    async_task & operator = (async_task &&) = default;

    async_task(const std::shared_ptr<_async_task_state<void>> & state)
      : state_(state) {
    }

#ifdef DEBUG
    ~async_task() noexcept(false) {
      vds_assert(!this->state_ || this->state_->is_processed());
    }
#endif

    template<class _Rep, class _Period>
    std::future_status wait_for(std::chrono::duration<_Rep, _Period> timeout) const {
      return this->state_->wait_for(timeout);
    }

    void get() {
      this->state_->get();
    }

    bool is_ready() const noexcept {
      return this->state_->is_ready();
    }

    void then(const std::function<void(void)> & f) {
      this->state_->then(f);
    }

    void detach() {
      auto s = std::move(this->state_);
      s->then([s]() {
        try {
          s->get();
        }
        catch (...) {
        }
      });
    }
  private:
    std::shared_ptr<_async_task_state<void>> state_;
  };

  template <typename result_type>
  class async_result {
  public:
    async_result()
      : state_(new _async_task_state<result_type>()) {
    }

    async_task<result_type> get_future() {
      return async_task<result_type>(this->state_);
    }

    template<typename init_type>
    void set_value(init_type && v) {
      this->state_->set_value(new _async_task_return_value<result_type>(std::forward<init_type>(v)));
    }

    void set_exception(std::exception_ptr ex) {
      this->state_->set_value(new _async_task_throw_exception<result_type>(ex));
    }

  private:
    std::shared_ptr<_async_task_state<result_type>> state_;
  };

  template <>
  class async_result<void> {
  public:
    async_result()
    : state_(new _async_task_state<void>()){
    }

    async_task<void> get_future() {
      return async_task<void>(this->state_);
    }

    void set_value() {
      this->state_->set_value(new _async_task_return_value<void>());
    }

    void set_exception(std::exception_ptr ex) {
      this->state_->set_value(new _async_task_throw_exception<void>(ex));
    }

  private:
    std::shared_ptr<_async_task_state<void>> state_;
  };


#ifndef _WIN32
  template<typename T>
  struct awaiter {
    vds::async_task<T> _future;
  public:
    explicit awaiter(vds::async_task<T> &&f) noexcept : _future(std::move(f)) {
    }

    bool await_ready() const noexcept {
      return _future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    template<typename U>
    void await_suspend(std::experimental::coroutine_handle<U> hndl) noexcept {
      this->_future.then([hndl]() mutable {
        hndl();
      });
    }

    T & await_resume() {
      return _future.get();
    }
  };

  template<>
  struct awaiter<void> {
    vds::async_task<void> _future;
  public:
    explicit awaiter(vds::async_task<void> &&f) noexcept
        : _future(std::move(f)) {}

    bool await_ready() const noexcept {
      return _future.is_ready();
    }

    template<typename U>
    void await_suspend(std::experimental::coroutine_handle<U> hndl) noexcept {
      this->_future.then([hndl]() mutable {
        hndl();
      });
    }

    void await_resume() { _future.get(); }
  };

  template<typename T>
  inline auto operator co_await(async_task<T> f) noexcept {
    return awaiter<T>(std::move(f));
  }

  inline auto operator co_await(async_task<void> f) noexcept {
    return awaiter<void>(std::move(f));
  }

#endif//_WIN32
}

namespace std {
  namespace experimental {
    template<typename R, typename... Args>
    struct coroutine_traits<vds::async_task<R>, Args...> {
      struct promise_type {
        vds::async_result<R> p;

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
#ifndef _WIN32
        void unhandled_exception() {
          p.set_exception(std::current_exception());
        }
#endif
      };
    };
    template<typename... Args>
    struct coroutine_traits<vds::async_task<void>, Args...> {
      struct promise_type {
        vds::async_result<void> p;

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

#ifndef _WIN32
        void unhandled_exception() {
          p.set_exception(std::current_exception());
        }
#endif
      };
    };
  };
}


#ifdef _WIN32
namespace std {
  template <typename  T>
  inline bool await_ready(vds::async_task<T> & _future)
  {
    return _future.is_ready();
  }

  template <typename  T>
  inline void await_suspend(vds::async_task<T> & _future,
    std::experimental::coroutine_handle<> _ResumeCb)
  {
    _future.then([_ResumeCb]() {
      _ResumeCb();
    });
  }

  template<typename T>
  inline T & await_resume(vds::async_task<T> & _future)
  {
    return (_future.get());
  }

  inline void await_resume(vds::async_task<void> & _future)
  {
    _future.get();
  }
}

#endif

#endif // __VDS_CORE_ASYNC_TASK_H_
 
