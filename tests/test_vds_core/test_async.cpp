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
