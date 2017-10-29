/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "tcp_network_socket.h"
#include "network_service.h"
#include "tcp_socket_server.h"
#include "logger.h"
#include "http_router.h"
#include "http_parser.h"
#include "http_middleware.h"
#include "http_serializer.h"
#include "http_request.h"
#include "barrier.h"
#include "async_buffer.h"
#include "const_data_buffer.h"
#include "file.h"
#include "asymmetriccrypto.h"
#include "ssl_tunnel.h"
#include "crypto_service.h"
#include "http_client.h"
#include "http_server.h"
#include "random_buffer.h"
#include "encoding.h"
#include "test_config.h"

struct shared_buffer
{
  uint8_t data_[1024];
};

static vds::async_task<std::string> read_answer(
  const vds::service_provider & sp,
  std::unique_ptr<shared_buffer> && buffer,
  const std::shared_ptr<vds::continuous_buffer<uint8_t>> & source,
  const std::string & result = std::string())
{
  return source->read_async(sp, buffer->data_, sizeof(buffer->data_))
    .then([sp, b = std::move(buffer), source, result](size_t readed) mutable -> vds::async_task<std::string> {
    if (0 == readed) {
      return [result]() { return result; };
    }
    else {
      auto value = result + std::string((const char *)b->data_, readed);
      return read_answer(sp, std::move(b), source, value);
    }
  });
}

TEST(http_tests, test_http_serializer)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(crypto_service);

  auto sp = registrator.build("test_http_serializer");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);


  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());

  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);

  vds::barrier b;

  vds::collect_data<uint8_t> cd;
  vds::http_serializer s(cd);
  
  s.write_async(sp, request)
  .execute(
    [&b, sp](const std::shared_ptr<std::exception> & ex) {
      if(!ex){
        b.set();
      } else {
        sp.unhandled_exception(ex);
      }
    });

  b.wait();
  //Wait
  registrator.shutdown(sp);

  std::string answer((const char *)cd.data(), cd.size());
  std::string expected = "GET / HTTP/1.0\nContent-Type: text/html; charset=utf-8\nContent-Length:" + std::to_string(test_data.length()) + "\n\n" + test_data;

  ASSERT_EQ(answer, expected);
}

TEST(http_tests, test_http_parser)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(crypto_service);

  auto sp = registrator.build("test_http_parser");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);


  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());

  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);

  std::string answer;
  vds::barrier b;

  vds::http_parser parser(
    [sp, &answer, &b](const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {
    if (!request) {
      return vds::async_task<>::empty();
    }

    return read_answer(
      sp,
      std::make_unique<shared_buffer>(),
      request->body())
    .then([&answer](const std::string & result){
      answer = result;
    });
  });

  vds::http_serializer serializer(parser);
  serializer.write_async(sp, request)
  .execute(
    [sp](const std::shared_ptr<std::exception> & ex) {
	  sp.unhandled_exception(ex);
  });

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, test_data);
}

