/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include <future>

static std::future<std::string> step1(
  int v)
{
  return std::async([v](){ return std::to_string(v); });
}

static std::future<std::string> step2(const std::string & v)
{
	return std::async([v]() { return "result" + v; });
}

static std::function<void(void)> step3_saved_done;

static std::future<std::string> step3(
	int v)
{
  std::promise<std::string> result;
  auto f = result.get_future();

  step3_saved_done = [r = std::move(result), v]() { r.set_value(std::to_string(v)); };

  return f;
}


TEST(code_tests, test_std::future) {
  auto t = step1(10).then([](const std::string & v) { return step2(v); });
  
  std::string test_result;
  t.execute(
    [&test_result](const std::shared_ptr<std::exception> & ex, const std::string & result){
      if(!ex){
        test_result = result;
      }else {
        FAIL() << ex->what();
      }
    });
  
  ASSERT_EQ(test_result, "result10");
}

TEST(code_tests, test_std::future1) {
  auto t = step1(10).then(
    [](const std::string & v) {
    return "result" + v;
  });

  std::string test_result;
  t.execute(
    [&test_result](const std::shared_ptr<std::exception> & ex, const std::string & result) {
      if(!ex){
      test_result = result;
      }
      else {
        FAIL() << ex->what();
      }
    });

  ASSERT_EQ(test_result, "result10");
}

static void test2(
  vds::barrier & b,
  std::string & test_result)
{
  auto t = step3(10).then(
    [](const std::string & v) {
    return "result" + v;
  });

  t.execute(
    [&test_result, &b](const std::shared_ptr<std::exception> & ex, const std::string & result) {
      if(!ex){
      test_result = result;
      }
      else {
        FAIL() << ex->what();
      }
      b.set();
    });
}

TEST(code_tests, test_std::future2) {
  vds::barrier b;
  std::string test_result;
  
  test2(b, test_result);
  
  step3_saved_done();
  
  b.wait();
  ASSERT_EQ(test_result, "result10");
  
}

