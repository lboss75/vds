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
      vds::stream_read<vds::udp_datagram>(server_socket.incoming()),
      vds::stream_write<vds::udp_datagram>(server_socket.outgoing())
    )(
    [](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Server closed");
    },
      [&error](const vds::service_provider & sp, std::exception_ptr ex) {
      error = ex;
    },
    sp);


    vds::barrier b;

  vds::udp_client client;
  auto client_socket = client.start(sp.create_scope("UDP client"));
  
  vds::udp_datagram response;
  client_socket.outgoing()->read_async(sp, &response, 1)
  .wait(
    [&b](const vds::service_provider & sp, size_t readed) {
        sp.get<vds::logger>()->debug(sp, "Client reader closed");
        b.set();
      },
    [&b](const vds::service_provider & sp, std::exception_ptr ex) {
        sp.get<vds::logger>()->debug(sp, "Client reader error");
        b.set();
      },
    sp.create_scope("Client reader"));

  
  const char data[] = "test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_";
  vds::udp_datagram request("127.0.0.1", 8000, data, sizeof(data) - 1);
  client_socket.outgoing()->write_all_async(sp, &request, 1)
  .wait(
    [](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Client writer closed");
      },
    [](const vds::service_provider & sp, std::exception_ptr ex) {
        sp.get<vds::logger>()->debug(sp, "Client writer error");
      },
    sp.create_scope("Client writer"));

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(std::string((const char *)response.data(), response.data_size()),
    "test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_");
  if (error) {
    GTEST_FAIL() << vds::exception_what(error);
  }
}
