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
        vds::dataflow(
          test_async_object::sync_method(obj),
          test_async_object::async_method(sp, obj)
        )
        (
         [&obj, &barrier]() {
            ASSERT_EQ(obj.state_, 2);
            obj.state_++;
            barrier.set();
          },
          [&barrier](std::exception_ptr ex) {
              FAIL() << vds::exception_what(ex);
              barrier.set();
          },
          10);
        
        barrier.wait();
    }

    ASSERT_EQ(obj.state_, 3);

    registrator.shutdown();
}

test_async_object::test_async_object()
    : state_(0)
{
}
