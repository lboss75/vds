/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_async.h"
#include "logger.h"
#include "barrier.h"
#include "mt_service.h"
#include "test_config.h"
#include <future>
#include <experimental/coroutine>
/*
// Void futures
template <typename... Args>
struct std::experimental::coroutine_traits<std::future<void>, Args...> {
struct promise_type {
  std::promise<void> p;
  auto get_return_object() { return p.get_future(); }
  std::experimental::suspend_never initial_suspend() { return {}; }
  std::experimental::suspend_never final_suspend() { return {}; }
  void set_exception(std::exception_ptr e) { p.set_exception(std::move(e)); }
  void unhandled_exception() { std::terminate(); }
  void return_void() { p.set_value(); }
};
};

// Non-void futures
template <typename R, typename... Args>
struct std::experimental::coroutine_traits<std::future<R>, Args...> {
struct promise_type {
  std::promise<R> p;
  auto get_return_object() { return p.get_future(); }
  std::experimental::suspend_never initial_suspend() { return {}; }
  std::experimental::suspend_never final_suspend() { return {}; }
  void set_exception(std::exception_ptr e) { p.set_exception(std::move(e)); }
  void unhandled_exception() { std::terminate(); }
  template <typename U> void return_value(U &&u) {
    p.set_value(std::forward<U>(u));
  }
};
};

template <typename R> auto operator co_await(std::future<R> &&f) {
  struct Awaiter {
    std::future<R> &&input;
    std::future<R> output;

    // better for app; may not be best for servers
    bool await_ready() {
//      if (input.is_ready () ) {
//        output = std::move (input);
//        return true;
//      }
      return false;
    }
    auto await_resume() { return output.get(); }
    void await_suspend(std::experimental::coroutine_handle<> coro) {
//      input.then([this, coro](auto result_future) mutable {
//        this->output = std::move(result_future);
//        coro.resume();
//      });
    }
  };
  return Awaiter{static_cast<std::future<R>&&>(f)};
}
*/

std::future<int> async_add(int a, int b)
{
  auto fut = std::async([=]() {
    int c = a + b;
    return c;
  });

  return fut;
}

std::future<int> async_fib(int n)
{
  if (n <= 2)
    co_return 1;

  int a = 1;
  int b = 1;

  // iterate computing fib(n)
  for (int i = 0; i < n - 2; ++i)
  {
    int c = co_await async_add(a, b);
    a = b;
    b = c;
  }

  co_return b;
}

int calc_fib(int n) {
  return async_fib(n).get();
}

TEST(mt_tests, test_async) {
  int result = calc_fib(5);

  GTEST_ASSERT_EQ(result, 5);
}
