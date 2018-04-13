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
#include "http_client.h"
#include "http_server.h"
#include "test_config.h"
#include "task_manager.h"

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
  return source->read_async(buffer->data_, sizeof(buffer->data_))
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

TEST(http_tests, test_server)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::task_manager task_manager;
  vds::network_service network_service;
  vds::file_logger file_logger(
    test_config::instance().log_level(),
    test_config::instance().modules());

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(task_manager);
  registrator.add(network_service);

  auto sp = registrator.build("test_server");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);

  //Start server
  vds::http_router router;
  router.add_static(
    "/",
    "<html><body>Hello World</body></html>");

  vds::http_server http_server;
  vds::http_middleware<vds::http_router> middleware(router);

  vds::barrier b;
  vds::tcp_socket_server server;
  server.start(
    sp,
    "127.0.0.1",
    8000,
    [&middleware, &http_server](const vds::service_provider & sp, const vds::tcp_network_socket & s) {

    http_server.start(sp,
      s.incoming(), s.outgoing(),
      [sp, &middleware](const std::shared_ptr<vds::http_message> & request) {
        return middleware.process(sp, request);
        })
      .execute(
        [sp](const std::shared_ptr<std::exception> & ex) {
          if(!ex){
            sp.get<vds::logger>()->debug("test", sp, "Connection closed");
          } else {
            sp.get<vds::logger>()->debug("test", sp, "Server error");
            sp.unhandled_exception(ex);
          }
        });
  }).execute(
      [sp, &b](const std::shared_ptr<std::exception> & ex) {
        if(!ex){
          sp.get<vds::logger>()->debug("test", sp, "Server has been started");
          b.set();
        } else {
          sp.get<vds::logger>()->debug("test", sp, "Server error");
          sp.unhandled_exception(ex);
          b.set();
        }
  });
  b.wait();
  b.reset();

  vds::http_client client;
  std::shared_ptr<vds::http_message> response;
  std::string answer;

  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [sp, &client, &response, &answer](const vds::tcp_network_socket & s) {

    sp.get<vds::logger>()->debug("test", sp, "Connected");

    return client.start(
      sp,
      s.incoming(), s.outgoing(),
      [sp, &response, &answer](const std::shared_ptr<vds::http_message> & request) {

      if (!request) {
        return vds::async_task<>::empty();
      }
      
      response = request;
      
      return read_answer(
          sp,
          std::make_unique<shared_buffer>(),
          request->body())
      .then(
        [&answer](const std::string & result){
            answer = result;
        });
    });
  })
  .execute(
    [sp, &b](const std::shared_ptr<std::exception> & ex) {
	  if(!ex){
        sp.get<vds::logger>()->debug("test", sp, "Request sent"); b.set();
	  }
	  else {
		  sp.get<vds::logger>()->debug("test", sp, "Request error");
		  b.set();
	  }
    });

  std::shared_ptr<vds::http_message> request = vds::http_request("GET", "/").get_message();
  request->body()->write_async(sp, nullptr, 0)
	.execute(
    [sp](const std::shared_ptr<std::exception> & ex) {
	  sp.unhandled_exception(ex);
  });


  client.send(sp, request)
  .then(
    [sp, &client]() {
      return client.send(sp, std::shared_ptr<vds::http_message>());
    })
  .execute(
    [sp](const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    });

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, "<html><body>Hello World</body></html>");

}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    
    test_config::instance().parse(argc, argv);
    
    return RUN_ALL_TESTS();
}


