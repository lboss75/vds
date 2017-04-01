/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include "test_async_task.h"

TEST(code_tests, test_async_task) {
  auto steps = vds::async_task(
    [](const std::function<void(const std::string &)> & done, const vds::error_handler & on_error, int v) {
      done(std::to_string(v));
    })->then(
    [](const std::function<void(const std::string &)> & done, const vds::error_handler & on_error, const std::string & v) {
      done("result" + v);
    });
  
  
  std::string test_result;
  steps->invoke(
    [&test_result](const std::string & result){
      test_result = result;
    },
    [](std::exception_ptr ex){
      FAIL() << vds::exception_what(ex);
    },
    10);
  
  ASSERT_EQ(test_result, "result10");
}
