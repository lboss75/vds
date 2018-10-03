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

    template<typename U>
    void await_suspend(std::experimental::coroutine_handle<U> hndl) noexcept {
      std::async([this, hndl]() mutable {
        this->_future.wait();
        hndl.resume();
      });
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
 
