/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include "test_async_task.h"

static vds::async_task<std::string> step1(
  int v)
{
  return [v](){ return std::to_string(v); };
}

static vds::async_task<std::string> step2(const std::string & v)
{
	auto f = [v]() { return "result" + v; };

	static_assert(
		false == vds::_async_task_functor_helper<decltype(&decltype(f)::operator())>::is_async_callback,
		"Is is_async_callback");

	return vds::async_task<std::string>(std::move(f));
}

static std::function<void(void)> step3_saved_done;

static vds::async_task<std::string> step3(
	int v)
{
	auto f = [v](const vds::async_result<std::string> & result) {
		step3_saved_done = [result, v]() { result.done(std::to_string(v)); };
	};

	static_assert(
		true == vds::_async_task_functor_helper<decltype(&decltype(f)::operator())>::is_async_callback,
		"Is not is_async_callback");

	return std::move(f);
}

static vds::async_task<std::string, int> step4(const std::string & v)
{
	return [v]() { return std::make_tuple("result" + v, v.length()); };
}

TEST(code_tests, test_async_task) {
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

TEST(code_tests, test_async_task1) {
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

TEST(code_tests, test_async_task2) {
  vds::barrier b;
  std::string test_result;
  
  test2(b, test_result);
  
  step3_saved_done();
  
  b.wait();
  ASSERT_EQ(test_result, "result10");
  
}

