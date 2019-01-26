/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include <future>
#include "test_config.h"

static vds::async_task<vds::expected<std::string>> step1(
  int v)
{
  auto r = std::make_shared<vds::async_result<vds::expected<std::string>>>();
  std::thread([=](){ r->set_value(std::to_string(v)); }).detach();
  return r->get_future();
}

static vds::async_task<vds::expected<std::string>> step2(const std::string & v)
{
  auto r = std::make_shared<vds::async_result<vds::expected<std::string>>>();
  std::thread([=]() { r->set_value("result" + v); }).detach();
  return r->get_future();
}

static std::function<void(void)> step3_saved_done;

static vds::async_task<vds::expected<std::string>> step3(
	int v)
{
  auto r = std::make_shared<vds::async_result<vds::expected<std::string>>>();
  auto f = r->get_future();

  step3_saved_done = [r, v]() { r->set_value(std::to_string(v)); };

  return f;
}


vds::async_task<vds::expected<std::string>> async_future(int v) {
  GET_EXPECTED_ASYNC(t, co_await step1(v));
  GET_EXPECTED_ASYNC(r, co_await step2(t));
  co_return r;
}


TEST(code_tests, async_future) {
  GET_EXPECTED_GTEST(test_result, async_future(10).get());
  ASSERT_EQ(test_result, "result10");
}

TEST(code_tests, async_future1) {
  auto test_result = step3(10);
  
  step3_saved_done();
  GET_EXPECTED_GTEST(tr, test_result.get());
  ASSERT_EQ(tr, "10");
}
