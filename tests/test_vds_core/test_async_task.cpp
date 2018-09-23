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
<<<<<<< HEAD
  auto r = std::make_shared<vds::async_result<std::string>>();
  auto f = r->get_future();
=======
  std::promise<std::string> result;
  auto f = result.get_future();
>>>>>>> ff78c01024632720d229efbec78a1748be3a1d2f

  //step3_saved_done = [r = std::move(result), v]() { r.set_value(std::to_string(v)); };

  return f;
}


<<<<<<< HEAD
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
=======
//TEST(code_tests, future) {
//  auto t = step1(10).then([](const std::string & v) { return step2(v); });
//
//  std::string test_result;
//  t.execute(
//    [&test_result](const std::shared_ptr<std::exception> & ex, const std::string & result){
//      if(!ex){
//        test_result = result;
//      }else {
//        FAIL() << ex->what();
//      }
//    });
//
//  ASSERT_EQ(test_result, "result10");
//}
//
//TEST(code_tests, test_std::future1) {
//  auto t = step1(10).then(
//    [](const std::string & v) {
//    return "result" + v;
//  });
//
//  std::string test_result;
//  t.execute(
//    [&test_result](const std::shared_ptr<std::exception> & ex, const std::string & result) {
//      if(!ex){
//      test_result = result;
//      }
//      else {
//        FAIL() << ex->what();
//      }
//    });
//
//  ASSERT_EQ(test_result, "result10");
//}
//
//static void test2(
//  vds::barrier & b,
//  std::string & test_result)
//{
//  auto t = step3(10).then(
//    [](const std::string & v) {
//    return "result" + v;
//  });
//
//  t.execute(
//    [&test_result, &b](const std::shared_ptr<std::exception> & ex, const std::string & result) {
//      if(!ex){
//      test_result = result;
//      }
//      else {
//        FAIL() << ex->what();
//      }
//      b.set();
//    });
//}
//
//TEST(code_tests, test_std::future2) {
//  vds::barrier b;
//  std::string test_result;
//
//  test2(b, test_result);
//
//  step3_saved_done();
//
//  b.wait();
//  ASSERT_EQ(test_result, "result10");
//
//}
//
>>>>>>> ff78c01024632720d229efbec78a1748be3a1d2f
