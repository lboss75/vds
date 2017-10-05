/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include "test_async_task.h"

static vds::async_task<const std::string &> step1(
  int v)
{
  return vds::create_async_task([v](){
    return std::to_string(v); 
  });
}

static vds::async_result<const std::string &> step2(const std::string & v)
{
  return vds::create_async_task(
    [v]() {
    return "result" + v;
  });
}

static std::function<void(void)> step3_saved_done;

static vds::async_result<const std::string &> step3(
  int v)
{
  return vds::create_async_task(
    [v](const vds::async_result<const std::string &> & done) {
      step3_saved_done = [done, v](){ done(std::to_string(v)); };
  });
}

TEST(code_tests, test_async_task) {
  vds::service_provider & sp = *(vds::service_provider *)nullptr;
  auto t = step1(10).then([](const std::string & v) { return step2(v); });
  
  std::string test_result;
  t
  .wait(
    [&test_result](const vds::service_provider & sp, const std::string & result){
      test_result = result;
    },
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      FAIL() << ex->what();
    },
    sp);
  
  ASSERT_EQ(test_result, "result10");
}

TEST(code_tests, test_async_task1) {
  vds::service_provider & sp = *(vds::service_provider *)nullptr;

  auto t = step1(10).then(
    [](
       const vds::service_provider & sp,
       const std::string & v)->void {
    return "result" + v;
  });

  std::string test_result;
  t.wait(
    [&test_result](const vds::service_provider & sp, const std::string & result) {
      test_result = result;
    },
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    FAIL() << ex->what();
  },
  sp);

  ASSERT_EQ(test_result, "result10");
  
  buffer f;
  async_for([](async_token)){
    s.read_async(f)
      .then(
        async_break(async_token);
      );
   );
   async_continue(async_token);
  }).wait().on_error();
}

static void test2(
  vds::barrier & b,
  std::string & test_result)
{
  vds::service_provider & sp = *(vds::service_provider *)nullptr;
  auto t = step3(10).then(
    [](
      const vds::service_provider & sp,
      const std::string & v)->void {
    done(sp, "result" + v);
  });

  t.wait(
    [&test_result, &b](const vds::service_provider & sp, const std::string & result) {
      test_result = result;
      b.set();
    },
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      FAIL() << ex->what();
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

