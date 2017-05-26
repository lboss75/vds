/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "service_provider.h"
#include "mt_service.h"
#include "network_service.h"
#include "dataflow.h"
#include "logger.h"
#include "file.h"
#include "barrier.h"
#include "udp_socket.h"

TEST(network_tests, test_udp_server)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);

  registrator.add(file_logger);
  registrator.add(mt_service);
  registrator.add(network_service);

  vds::barrier done;

  auto sp = registrator.build("network_tests::test_udp_server");
  registrator.start(sp);

  std::exception_ptr error;
  vds::udp_server server;

  auto server_socket = server.start(sp, "127.0.0.1", 8000);

    vds::dataflow(
      vds::stream_read(server_socket.incoming()),
      vds::stream_write(server_socket.outgoing())
    )(
    [](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Server closed");
    },
      [&error](const vds::service_provider & sp, std::exception_ptr ex) {
      error = ex;
    },
    sp);


    vds::barrier b;
    std::string answer;

  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [&b, &answer](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        vds::tcp_network_socket && s) {

    sp.get<vds::logger>()->debug(sp, "Connected");
    vds::cancellation_token_source cancellation;

    vds::async_series(
      vds::create_async_task(
        [s, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {

      const char data[] = "test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_\n";

      vds::dataflow(
        vds::dataflow_arguments<uint8_t>((const uint8_t *)data, sizeof(data) - 1),
        vds::write_tcp_network_socket(s, cancellation.token())
      )(
        [done](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Client writer closed");
        done(sp);
      },
        [on_error](const vds::service_provider & sp, std::exception_ptr ex) {
        sp.get<vds::logger>()->debug(sp, "Client writer error");
        on_error(sp, ex);
      },
        sp.create_scope("Client writer"));

    }),
      vds::create_async_task(
        [s, &answer, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::read_tcp_network_socket(s, cancellation.token()),
        read_for_newline(
          [&answer, s, cancellation](const vds::service_provider & sp, const std::string & value) {
        answer = value;
        cancellation.cancel();
      }
        )
      )(
        [done](const vds::service_provider & sp) {
        done(sp);
      },
        [on_error](const vds::service_provider & sp, std::exception_ptr ex) {
        sp.get<vds::logger>()->debug(sp, "Client reader closed");
        on_error(sp, ex);
      },
        sp.create_scope("Client read dataflow"));

    })
      ).wait(
        [done](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Client reader closed");
      done(sp);
    },
        [on_error](const vds::service_provider & sp, std::exception_ptr ex) {
      sp.get<vds::logger>()->debug(sp, "Client reader error");
      on_error(sp, ex);
    },
      sp.create_scope("Client reader"));
  }).wait(
    [&b](const vds::service_provider & sp) {
    sp.get<vds::logger>()->debug(sp, "Request sent");
    b.set();
  },
    [&b](const vds::service_provider & sp, std::exception_ptr ex) {
    sp.get<vds::logger>()->debug(sp, "Request error");
    b.set();
  },
    sp.create_scope("Client"));

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, "test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_");
  if (error) {
    GTEST_FAIL() << vds::exception_what(error);
  }
}
