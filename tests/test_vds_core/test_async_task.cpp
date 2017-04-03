/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include "test_async_task.h"

static vds::async_task<void(const std::string &)> step1(int v)
{
  return vds::create_async_task(
    [v](const std::function<void(const std::string &)> & done, const vds::error_handler & on_error) {
    done(std::to_string(v));
  });
}

static vds::async_task<void(const std::string &)> step2(const std::string & v)
{
  return vds::create_async_task(
    [v](const std::function<void(const std::string &)> & done, const vds::error_handler & on_error) {
    done("result" + v);
  });
}

TEST(code_tests, test_async_task) {
  auto t = step1(10).then([](const std::string & v) { return step2(v); });
  
  std::string test_result;
  t.wait(
    [&test_result](const std::string & result){
      test_result = result;
    },
    [](std::exception_ptr ex) {
      FAIL() << vds::exception_what(ex);
    }
  );
  
  ASSERT_EQ(test_result, "result10");
}

TEST(code_tests, test_async_task1) {
  auto t = step1(10).then(
    [](const std::function<void(const std::string &)> & done, const vds::error_handler & on_error, const std::string & v)->void {
    done("result" + v);
  });

  std::string test_result;
  t.wait(
    [&test_result](const std::string & result) {
    test_result = result;
  },
    [](std::exception_ptr ex) {
    FAIL() << vds::exception_what(ex);
  }
  );

  ASSERT_EQ(test_result, "result10");
}

