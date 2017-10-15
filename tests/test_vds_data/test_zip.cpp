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
    auto sp = registrator.build("test_symmetric");
    registrator.start(sp);

    random_buffer buffer;
    
    compare_data<uint8_t> cd(buffer.data(), buffer.size());
    vds::inflate il(cd);
    vds::deflate dl(il);
    random_stream<uint8_t> rs(dl);
    
    rs.write(buffer.data(), buffer.size());
    rs.final();    
      
    registrator.shutdown(sp);
  }
}
