/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "random_buffer.h"
#include "random_reader.h"
#include "compare_data.h"

TEST(test_zip, inflate_tests) {
  vds::service_registrator registrator;
  vds::mt_service mt_service;

  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(mt_service);
  registrator.add(console_logger);
  {
    auto sp = registrator.build("test_symmetric");
    registrator.start(sp);

    random_buffer buffer;

    vds::barrier b;
    std::exception_ptr error;
    dataflow(
      random_reader<uint8_t>(buffer.data(), buffer.size()),
      vds::deflate(),
      vds::inflate(),
      compare_data<uint8_t>(buffer.data(), buffer.size())
    )(
      [&b](const vds::service_provider &) {
        b.set(); 
      },
      [&b, &error](const vds::service_provider &, std::exception_ptr ex) {
        error = ex;
        b.set();
      },
      sp);

    b.wait();
    registrator.shutdown(sp);
    if (error) {
      GTEST_FAIL() << vds::exception_what(error);
    }
  }
}
