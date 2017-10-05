///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "stdafx.h"
//#include "test_async.h"
//#include "logger.h"
//#include "barrier.h"
//#include "mt_service.h"
//#include "test_config.h"
//
//TEST(mt_tests, test_async) {
//    vds::service_registrator registrator;
//
//    vds::console_logger logger(
//      test_config::instance().log_level(),
//      test_config::instance().modules());
//    vds::mt_service mt_service;
//
//    registrator.add(logger);
//    registrator.add(mt_service);
//
//    test_async_object obj;
//    vds::barrier barrier;
//    std::shared_ptr<std::exception> error;
//
//    {
//      auto sp = registrator.build("test_async");
//      registrator.start(sp);
//      vds::mt_service::enable_async(sp);
//      
//      vds::dataflow(
//        test_async_object::source_method(obj),
//        test_async_object::sync_method(obj),
//        test_async_object::async_method(obj),
//        test_async_object::target_method(obj)
//      )
//      .wait
//      (
//        [&obj, &barrier](const vds::service_provider & sp) {
//          barrier.set();
//        },
//        [&barrier, &error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
//          error = ex;
//          barrier.set();
//        },
//        sp);
//
//      barrier.wait();
//
//      registrator.shutdown(sp);
//    }
//    if (error) {
//      GTEST_FAIL() << error->what();
//    }
//}
//
//test_async_object::test_async_object()
//{
//}
