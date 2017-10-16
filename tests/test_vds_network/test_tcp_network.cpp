/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "tcp_network_socket.h"
#include "service_provider.h"
#include "mt_service.h"
#include "network_service.h"
#include "logger.h"
#include "file.h"
#include "barrier.h"
#include "tcp_socket_server.h"
#include "random_buffer.h"
#include "random_stream.h"
#include "compare_data.h"
#include "test_config.h"
#include "task_manager.h"

TEST(network_tests, test_server)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::task_manager task_manager;
  vds::network_service network_service;
  vds::file_logger file_logger(
    test_config::instance().log_level(),
    test_config::instance().modules());

  registrator.add(file_logger);
  registrator.add(task_manager);
  registrator.add(mt_service);
  registrator.add(network_service);

  vds::barrier done;

  auto sp = registrator.build("network_tests::test_server");
  registrator.start(sp);

  vds::imt_service::enable_async(sp);

  std::shared_ptr<std::exception> error;
  vds::barrier b;
  vds::tcp_socket_server server;
  server.start(
    sp,
    "127.0.0.1",
    8000,
    [&error](const vds::service_provider & sp, const vds::tcp_network_socket & s) {
    vds::copy_stream<uint8_t>(sp, s.incoming(), s.outgoing())
    .wait(
      [s, sp]() {
        sp.get<vds::logger>()->debug("TCP", sp, "Server closed");
      },
      [&error](const std::shared_ptr<std::exception> & ex) {
        error = ex;
      });
    }).wait(
    [&b, sp]() {
      sp.get<vds::logger>()->debug("TCP", sp, "Server has been started");
      b.set();
    },
    [&b, &error](const std::shared_ptr<std::exception> & ex) {
      error = ex;
      b.set();
    });

  b.wait();

  if (error) {
    registrator.shutdown(sp);
    GTEST_FAIL() << error->what();
  }
  
  b.reset();

  std::string answer;
  random_buffer data;

  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [&b, &answer, &data, sp](
        const vds::tcp_network_socket & s) {

    sp.get<vds::logger>()->debug("TCP", sp, "Connected");

    return vds::async_series(
      vds::dataflow(
        random_reader<uint8_t>(data.data(), data.size()),
        vds::stream_write<vds::continuous_stream<uint8_t>>(s.outgoing())
      ),
      vds::dataflow(
        vds::stream_read<vds::continuous_stream<uint8_t>>(s.incoming()),
        compare_data<uint8_t>(data.data(), data.size())
      ));
  }).wait(
    [&b, sp]() {
      sp.get<vds::logger>()->debug("TCP", sp, "Request sent");
      b.set();
    },
    [&b, &error, sp](const std::shared_ptr<std::exception> & ex) {
      error = ex;
      sp.get<vds::logger>()->debug("TCP", sp, "Request error");
      b.set();
    });

  b.wait();
  //Wait
  registrator.shutdown(sp);

  if (error) {
    GTEST_FAIL() << error->what();
  }
}
