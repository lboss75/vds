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
    
    random_reader<uint8_t> reader(buffer.data(), buffer.size());
    
    vds::deflate dl;
    vds::inflate il;
    
    uint8_t buf1[1024];
    uint8_t buf2[1024];
    uint8_t buf3[1024];
    for(;;){
      auto l = reader.read(buf1, sizeof(buf1));
      
      if(0 == l){
        break;
      }
      
      auto p = buf1;
      while(0 < l){
        size_t readed;
        size_t written;
        dl.update(p, l, buf2, sizeof(buf2), readed, written);
        
        l -= readed;
        p += readed;
        
        if(0 < written){
          il.update(buf2, written, buf3, sizeof(buf3), );
        }
      }
      
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
