/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "random_buffer.h"
#include "random_reader.h"
#include "compare_data.h"
#include "test_config.h"

TEST(test_zip, inflate_tests) {
  vds::service_registrator registrator;
  vds::mt_service mt_service;

  vds::console_logger console_logger(
      test_config::instance().log_level(),
      test_config::instance().modules());

  registrator.add(mt_service);
  registrator.add(console_logger);
  {
    auto sp = registrator.build("test_symmetric");
    registrator.start(sp);

    random_buffer buffer;

    vds::barrier b;
    std::shared_ptr<std::exception> error;
    dataflow(
      random_reader<uint8_t>(buffer.data(), buffer.size()),
      vds::deflate(),
      vds::inflate(),
      compare_data<uint8_t>(buffer.data(), buffer.size())
    )
    .wait(
      [&b](const vds::service_provider &) {
        b.set(); 
      },
      [&b, &error](const vds::service_provider &, const std::shared_ptr<std::exception> & ex) {
        error = ex;
        b.set();
      },
      sp);

    b.wait();
    registrator.shutdown(sp);
    if (error) {
      GTEST_FAIL() << error->what();
    }
  }
}
