/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_async.h"


TEST(mt_tests, test_async) {
    vds::service_registrator registrator;

    vds::mt_service mt;
    vds::console_logger logger;

    registrator.add(logger);
    registrator.add(mt);

    test_async_object obj;
    vds::barrier barrier;

    {
        auto sp = registrator.build();
        auto done_handler = vds::lambda_handler(
         [&obj, &barrier]() {
            ASSERT_EQ(obj.state_, 2);
            obj.state_++;
            barrier.set();
        });
        auto error_handler = vds::lambda_handler(
          [&barrier](std::exception * ex) {
              FAIL() << ex->what();
              delete ex;
              barrier.set();
          });
        vds::sequence(
          test_async_object::sync_method(obj),
          test_async_object::async_method(sp, obj),
          done_handler
        )
        (
          error_handler,
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
