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
  vds::imt_service::enable_async(sp);

  std::shared_ptr<std::exception> error;
  vds::udp_server server;

  auto server_socket = server.start(sp, "127.0.0.1", 8000);

    vds::dataflow(
      vds::stream_read<vds::continuous_stream<vds::udp_datagram>>(server_socket.incoming()),
      vds::stream_write<vds::async_stream<vds::udp_datagram>>(server_socket.outgoing())
    )(
    [&server_socket](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Server closed");
      server_socket.close();
    },
      [&error, &server_socket](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      error = ex;
      server_socket.close();
    },
    sp);


    vds::barrier b;

  vds::udp_client client;
  auto client_socket = client.start(sp.create_scope("UDP client"));
  
  vds::udp_datagram response;
  client_socket.incoming()->read_async(sp, &response, 1)
  .wait(
    [&b, &client_socket](const vds::service_provider & sp, size_t readed) {
        sp.get<vds::logger>()->debug(sp, "Client reader closed");
        client_socket.close();
        b.set();
      },
    [&b, &client_socket](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        sp.get<vds::logger>()->debug(sp, "Client reader error");
        client_socket.close();
        b.set();
      },
    sp.create_scope("Client reader"));

  
  const char data[] = "test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_";
  vds::udp_datagram request("127.0.0.1", 8000, data, sizeof(data) - 1);
  auto stream = client_socket.outgoing();
  stream->write_value_async(sp, request)
  .wait([stream](const vds::service_provider & sp){},
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex){
        }, sp);

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(std::string((const char *)response.data(), response.data_size()),
    "test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_");
  if (error) {
    GTEST_FAIL() << error->what();
  }
}
