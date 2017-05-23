/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_async.h"
#include "logger.h"
#include "barrier.h"
#include "mt_service.h"

TEST(mt_tests, test_async) {
    vds::service_registrator registrator;

    vds::console_logger logger(vds::ll_trace);
    vds::mt_service mt_service;

    registrator.add(logger);
    registrator.add(mt_service);

    test_async_object obj;
    vds::barrier barrier;

    {
      auto sp = registrator.build("test_async");
      registrator.start(sp);
      
      vds::dataflow(
        test_async_object::source_method(obj),
        test_async_object::sync_method(obj),
        test_async_object::async_method(obj),
        test_async_object::target_method(obj)
      )
      (
        [&obj, &barrier](const vds::service_provider & sp) {
        barrier.set();
      },
        [&barrier](const vds::service_provider & sp, std::exception_ptr ex) {
        FAIL() << vds::exception_what(ex);
        barrier.set();
      },
        sp
      );

      barrier.wait();

      registrator.shutdown(sp);
    }
}

test_async_object::test_async_object()
{
}
