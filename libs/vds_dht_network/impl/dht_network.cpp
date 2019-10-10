/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "dht_network.h"
#include "udp_transport.h"
#include "db_model.h"
#include "keys_control.h"
#include "node_info_dbo.h"
#include "dht_object_id.h"
#include "device_config_dbo.h"

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

vds::expected<std::list<vds::const_data_buffer>> vds::dht::network::service::select_near(const database_read_transaction& t, const const_data_buffer& target, size_t count)
{
  std::list<std::tuple<vds::const_data_buffer, vds::const_data_buffer>> cache;

  orm::node_info_dbo t1;
  GET_EXPECTED(st, t.get_reader(t1.select(t1.node_id).where(t1.last_activity > std::chrono::system_clock::now() - std::chrono::hours(72))));
  WHILE_EXPECTED(st.execute()) {
    auto distance = dht::dht_object_id::distance(t1.node_id.get(st), target);
    auto value = std::make_tuple(distance, t1.node_id.get(st));
    cache.insert(std::lower_bound(cache.begin(), cache.end(), value, [](auto l, auto r) { return std::get<0>(l) < std::get<0>(r);  }), value);
    while (count < cache.size()) {
      cache.pop_back();
    }
  }
  WHILE_EXPECTED_END()

  std::list<vds::const_data_buffer> result;
  for (const auto& item : cache) {
    result.push_back(std::get<0>(item));
  }

  return result;
}

