/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include <future>

static vds::async_task<std::string> step1(
  int v)
{
  return std::async([v](){ return std::to_string(v); });
}

static vds::async_task<std::string> step2(const std::string & v)
{
	return std::async([v]() { return "result" + v; });
}

static std::function<void(void)> step3_saved_done;

static vds::async_task<std::string> step3(
	int v)
{
  auto r = std::make_shared<vds::async_result<std::string>>();
  auto f = r->get_future();

  step3_saved_done = [r, v]() { r->set_value("result" + std::to_string(v)); };

  return f;
}


vds::async_task<std::string> async_future(int v) {
  auto t = co_await step1(v);
  auto r = co_await step2(t);
  co_return r;
}


TEST(code_tests, async_future) {
  auto test_result = async_future(10).get();
  ASSERT_EQ(test_result, "result10");
}

TEST(code_tests, async_future1) {
  auto test_result = step3(10);
  
  step3_saved_done();

  ASSERT_EQ(test_result.get(), "result10");
}
