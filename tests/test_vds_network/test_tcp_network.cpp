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

class echo_stream : public vds::stream<uint8_t> {
public:
  echo_stream(const vds::tcp_network_socket & s)
    : vds::stream<uint8_t>(new _echo_stream(s)) {
  }

private:
  class _echo_stream : public vds::_stream<uint8_t> {
  public:
    _echo_stream(const vds::tcp_network_socket & s)
      : s_(s) {
    }

    void write(
      const uint8_t *data,
      size_t len) override {
      this->s_.write_async(data, len).wait();
    }

  private:
    vds::tcp_network_socket s_;
  };
};

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

  vds::tcp_socket_server server;
  server.start(
    sp,
    vds::network_address::any_ip4(8000),
    [sp](vds::tcp_network_socket s) {
    s.start(sp, echo_stream(s));
  }).get();


  std::string answer;
  random_buffer data;

  const auto cd = std::make_shared<compare_data<uint8_t>>(data.data(), data.size());
  auto s = vds::tcp_network_socket::connect(
    sp,
    vds::network_address::any_ip4(8000),
    *cd);

  sp.get<vds::logger>()->debug("TCP", sp, "Connected");


  auto rs = std::make_shared<random_stream_async<uint8_t>>(s);

  rs->write_async(data.data(), data.size()).get();
  rs->write_async(nullptr, 0).get();

  //Wait
  registrator.shutdown(sp);
}
