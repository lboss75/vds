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

vds::async_task<vds::expected<void>> copy_stream(
  std::shared_ptr<vds::stream_input_async<uint8_t>> reader,
  std::shared_ptr<vds::stream_output_async<uint8_t>> writer) {
  auto buffer = std::shared_ptr<uint8_t>(new uint8_t[1024]);
  for (;;) {
    size_t readed;
    GET_EXPECTED_VALUE_ASYNC(readed, co_await reader->read_async(buffer.get(), 1024));
    if (0 == readed) {
      CHECK_EXPECTED_ASYNC(co_await writer->write_async(nullptr, 0));
      co_return vds::expected<void>();
    }

    CHECK_EXPECTED_ASYNC(co_await writer->write_async(buffer.get(), readed));
  }
}

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

  GET_EXPECTED_GTEST(sp, registrator.build());
  CHECK_EXPECTED_GTEST(registrator.start());

  vds::tcp_socket_server server;
  CHECK_EXPECTED_GTEST(server.start(
    sp,
    vds::network_address::any_ip4(8000),
    [sp](const std::shared_ptr<vds::tcp_network_socket> & s) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
      GET_EXPECTED_ASYNC(writer, s->get_output_stream(sp));

      co_return writer;
  }).get());
  
  std::string answer;
  random_buffer data;

  GET_EXPECTED_GTEST(address, vds::network_address::tcp_ip4("localhost", 8000));
  GET_EXPECTED_GTEST(s, vds::tcp_network_socket::connect(sp, address));

  sp->get<vds::logger>()->debug("TCP", "Connected");
  GET_EXPECTED_GTEST(writer, s->get_output_stream(sp));

  std::thread t1([sp, writer, &data]() {
    auto rs = std::make_shared<random_stream<uint8_t>>(writer);
    (void)rs->write_async(data.data(), data.size()).get();
    (void)rs->write_async(nullptr, 0).get();
  });

  GET_EXPECTED_GTEST(reader, s->get_input_stream(sp));

  std::thread t2([sp, reader, &data]() {
    const auto cd = std::make_shared<compare_data_async<uint8_t>>(data.data(), data.size());
    (void)copy_stream(reader, cd).get();
  });

  t1.join();
  t2.join();

  //Wait
  CHECK_EXPECTED_GTEST(registrator.shutdown());
}
