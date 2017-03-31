/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_async.h"

TEST(mt_tests, test_async) {
    vds::service_registrator registrator;

    vds::console_logger logger(vds::ll_trace);
    vds::mt_service mt_service;

    registrator.add(logger);
    registrator.add(mt_service);

    test_async_object obj;
    vds::barrier barrier;

    {
        auto sp = registrator.build();
        auto t = vds::sequence(
          test_async_object::sync_method(obj),
          test_async_object::async_method(sp, obj)
        );

        auto f = t.get_feature();

        t(10);

        f.get();

        ASSERT_EQ(obj.state_, 2);
    }

    ASSERT_EQ(obj.state_, 3);

    registrator.shutdown();
}

class test
{
public:

  template<typename context_type, void(const void *, size_t)>
  class handler
  {
  public:

    void operator()(int value)
    {
    }
  };
};


test_async_object::test_async_object()
    : state_(0)
{
}
