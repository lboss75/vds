/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include "test_async_task.h"

static vds::async_task<const std::string &> step1(int v)
{
  return vds::create_async_task(
    [v](const std::function<void(const vds::service_provider & sp, const std::string &)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
    done(sp, std::to_string(v));
  });
}

static vds::async_task<const std::string &> step2(const std::string & v)
{
  return vds::create_async_task(
    [v](const std::function<void(const vds::service_provider & sp, const std::string &)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
    done(sp, "result" + v);
  });
}

static std::function<void(void)> step3_saved_done;

static vds::async_task<const std::string &> step3(int v)
{
  return vds::create_async_task(
    [v](const std::function<void(const vds::service_provider &, const std::string &)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      step3_saved_done = [&sp, done, v](){done(sp, std::to_string(v));};
  });
}

TEST(code_tests, test_async_task) {
  vds::service_provider & sp = *(vds::service_provider *)nullptr;
  auto t = step1(10).then([](const vds::service_provider & sp, const std::string & v) { return step2(v); });
  
  std::string test_result;
  t.wait(
    [&test_result](const vds::service_provider & sp, const std::string & result){
      test_result = result;
    },
    [](const vds::service_provider & sp, std::exception_ptr ex) {
      FAIL() << vds::exception_what(ex);
    },
    sp);
  
  ASSERT_EQ(test_result, "result10");
}

TEST(code_tests, test_async_task1) {
  vds::service_provider & sp = *(vds::service_provider *)nullptr;

  auto t = step1(10).then(
    [](const std::function<void(const vds::service_provider & sp, const std::string &)> & done,
       const vds::error_handler & on_error,
       const vds::service_provider & sp,
       const std::string & v)->void {
    done(sp, "result" + v);
  });

  std::string test_result;
  t.wait(
    [&test_result](const vds::service_provider & sp, const std::string & result) {
      test_result = result;
    },
    [](const vds::service_provider & sp, std::exception_ptr ex) {
    FAIL() << vds::exception_what(ex);
  },
  sp);

  ASSERT_EQ(test_result, "result10");
}

static void test2(
  vds::barrier & b,
  std::string & test_result)
{
  vds::service_provider & sp = *(vds::service_provider *)nullptr;
  auto t = step3(10).then(
    [](const std::function<void(const vds::service_provider & sp, const std::string &)> & done,
      const vds::error_handler & on_error,
      const vds::service_provider & sp,
      const std::string & v)->void {
    done(sp, "result" + v);
  });

  t.wait(
    [&test_result, &b](const vds::service_provider & sp, const std::string & result) {
      test_result = result;
      b.set();
    },
    [](const vds::service_provider & sp, std::exception_ptr ex) {
      FAIL() << vds::exception_what(ex);
    },
    sp);
}

TEST(code_tests, test_async_task2) {
  vds::barrier b;
  std::string test_result;
  
  test2(b, test_result);
  
  step3_saved_done();
  
  b.wait();
  ASSERT_EQ(test_result, "result10");
  
}

