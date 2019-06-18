/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <device_config_dbo.h>
#include "stdafx.h"
#include "dht_network.h"
#include "private/udp_transport.h"
#include "db_model.h"
#include "keys_control.h"

vds::expected<void> vds::dht::network::service::register_services(service_registrator& registrator) {
  registrator.add_service<client>(&this->client_);
  return expected<void>();
}

vds::expected<void> vds::dht::network::service::start(
  const service_provider * sp,
  const std::shared_ptr<iudp_transport> & udp_transport,
  const uint16_t port,
  bool dev_network) {

  std::list<std::function<async_task<expected<void>>()>> final_tasks;
  CHECK_EXPECTED(sp->get<db_model>()->async_transaction([this, sp, udp_transport, port, dev_network, &final_tasks](database_transaction& t) -> expected<void> {
    CHECK_EXPECTED(this->client_.load_keys(t));

    final_tasks.push_back([
      this,
        sp,
        port,
        udp_transport,
        dev_network]()->async_task<expected<void>> {
      CHECK_EXPECTED_ASYNC(this->client_.start(
        sp,
        udp_transport,
        port,
        dev_network));
      co_return expected<void>();
    });
    return expected<void>();
  }).get());

  while (!final_tasks.empty()) {
    CHECK_EXPECTED(final_tasks.front()().get());
    final_tasks.pop_front();
  }

  return expected<void>();
}

vds::expected<void> vds::dht::network::service::stop() {
  CHECK_EXPECTED(this->client_.stop());
  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::dht::network::service::prepare_to_stop() {
  co_return expected<void>();
}

