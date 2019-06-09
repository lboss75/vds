/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <device_config_dbo.h>
#include "stdafx.h"
#include "dht_network.h"
#include "private/udp_transport.h"
#include "db_model.h"
#include "current_config_dbo.h"
#include "cert_control.h"

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
    std::shared_ptr<asymmetric_public_key> node_public_key;
    std::shared_ptr<asymmetric_private_key> node_key;

    orm::current_config_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.public_key, t1.private_key)));
    GET_EXPECTED(st_execute, st.execute());
    if (st_execute) {
      GET_EXPECTED(node_public_key_data, asymmetric_public_key::parse_der(t1.public_key.get(st)));
      node_public_key = std::make_shared<asymmetric_public_key>(std::move(node_public_key_data));

      GET_EXPECTED(node_key_data, asymmetric_private_key::parse_der(t1.private_key.get(st), std::string()));
      node_key = std::make_shared<asymmetric_private_key>(std::move(node_key_data));
    }
    else {
      GET_EXPECTED(node_key_data, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
      node_key = std::make_shared<asymmetric_private_key>(std::move(node_key_data));

      GET_EXPECTED(public_key, asymmetric_public_key::create(*node_key));
      GET_EXPECTED(key_id, public_key.hash(hash::sha256()));

      node_public_key = std::make_shared<asymmetric_public_key>(std::move(public_key));
      GET_EXPECTED(node_public_key_data, node_public_key->der());
      GET_EXPECTED(node_key_der, node_key->der(std::string()));
      CHECK_EXPECTED(t.execute(t1.insert(
        t1.public_key = node_public_key_data,
        t1.private_key = node_key_der)));
    }

    final_tasks.push_back([
      this,
        sp,
        node_public_key,
        node_key,
        port,
        udp_transport,
        dev_network]()->async_task<expected<void>> {
      this->udp_transport_task_ = std::make_unique<async_task<expected<void>>>(udp_transport->start(
        sp,
        node_public_key,
        node_key,
        port,
        dev_network));

      CHECK_EXPECTED_ASYNC(this->client_.start(
        sp,
        node_public_key,
        node_key,
        udp_transport));
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
  if (this->udp_transport_task_) {
    CHECK_EXPECTED(this->udp_transport_task_->get());
    this->udp_transport_task_.reset();
  }

  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::dht::network::service::prepare_to_stop() {
  co_return expected<void>();
}

