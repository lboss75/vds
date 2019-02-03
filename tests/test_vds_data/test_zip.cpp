/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "random_buffer.h"
#include "random_stream.h"
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
    CHECK_EXPECTED_GTEST(registrator.build());
    CHECK_EXPECTED_GTEST(registrator.start());

    random_buffer buffer;
    
    auto cd = std::make_shared<compare_data_async<uint8_t>>(buffer.data(), buffer.size());
    GET_EXPECTED_GTEST(il, vds::inflate::create(cd));

    GET_EXPECTED_GTEST(dl, vds::deflate::create(il));

    auto rs = std::make_shared<random_stream<uint8_t>>(dl);
    
    CHECK_EXPECTED_GTEST(rs->write_async(buffer.data(), buffer.size()).get());
    CHECK_EXPECTED_GTEST(rs->write_async(nullptr, 0).get());

    CHECK_EXPECTED_GTEST(registrator.shutdown());
  }
}
