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
#include "async_stream.h"
#include "const_data_buffer.h"
#include "file.h"
#include "http_client.h"
#include "http_server.h"

TEST(http_tests, test_server)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);

  registrator.add(mt_service);
  registrator.add(file_logger);
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
      [&middleware](
        const vds::service_provider & sp,
        const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

        return middleware.process(sp, request);
        })
      .wait(
        [](const vds::service_provider & sp) {
          sp.get<vds::logger>()->debug(sp, "Connection closed");
        },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          sp.get<vds::logger>()->debug(sp, "Server error");
          sp.unhandled_exception(ex);
        },
        sp);
  }).wait(
      [&b](const vds::service_provider & sp) {
    sp.get<vds::logger>()->debug(sp, "Server has been started");
    b.set();
  },
      [&b](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    sp.get<vds::logger>()->debug(sp, "Server error");
    sp.unhandled_exception(ex);
    b.set();
  },
    sp
    );
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
      [&client, &response, &answer](
        const vds::service_provider & sp,
        const vds::tcp_network_socket & s) {

    sp.get<vds::logger>()->debug(sp, "Connected");

    return client.start(
      sp,
      s.incoming(), s.outgoing(),
      [&response, &answer](
        const vds::service_provider & sp,
        const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {

      response = request;

      return vds::create_async_task(
        [&response, &answer](
          const std::function<void(const vds::service_provider & sp)> & task_done,
          const vds::error_handler & on_error,
          const vds::service_provider & sp)
      {
        auto data = std::make_shared<std::vector<uint8_t>>();
        vds::dataflow(
          vds::stream_read<vds::continuous_stream<uint8_t>>(response->body()),
          vds::collect_data(*data)
        )(
          [data, &answer, task_done](const vds::service_provider & sp) {
          answer = std::string((const char *)data->data(), data->size());
          task_done(sp);
        },
          on_error,
          sp.create_scope("Client read dataflow"));

      });
    });
  })
  .wait(
    [&b](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Request sent"); b.set();
    },
    [&b](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.get<vds::logger>()->debug(sp, "Request error");
      b.set(); 
    },
    sp.create_scope("Client"));

  std::shared_ptr<vds::http_message> request = vds::http_request("GET", "/").get_message();
  request->body()->write_all_async(sp, nullptr, 0).wait(
    [](const vds::service_provider & sp) {},
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {},
    sp);


  client.send(sp, request)
  .then(
    [&client](const vds::service_provider & sp) {
    return client.send(sp, std::shared_ptr<vds::http_message>());
    })
  .wait(
    [](const vds::service_provider & sp) {},
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    },
  sp);

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, "<html><body>Hello World</body></html>");

}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


