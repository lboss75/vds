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
    std::shared_ptr<certificate> node_cert;
    std::shared_ptr<asymmetric_private_key> node_key;

      orm::current_config_dbo t1;
      GET_EXPECTED(st, t.get_reader(t1.select(t1.cert, t1.cert_key)));
      GET_EXPECTED(st_execute, st.execute());
      if (st_execute) {
        GET_EXPECTED(node_cert_data, certificate::parse_der(t1.cert.get(st)));
        node_cert = std::make_shared<certificate>(std::move(node_cert_data));

        GET_EXPECTED(node_key_data, asymmetric_private_key::parse_der(t1.cert_key.get(st), std::string()));
        node_key = std::make_shared<asymmetric_private_key>(std::move(node_key_data));
      }
      else {
        GET_EXPECTED(node_key_data, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
        node_key = std::make_shared<asymmetric_private_key>(std::move(node_key_data));

        GET_EXPECTED(public_key, asymmetric_public_key::create(*node_key));

        auto common_node_cert = cert_control::get_storage_certificate();
        auto common_node_key = cert_control::get_common_storage_private_key();
        certificate::create_options options;
      options.name = "!Node Cert";
      options.country = "RU";
      options.organization = "IVySoft";
      options.ca_certificate = common_node_cert.get();
      options.ca_certificate_private_key = common_node_key.get();

      GET_EXPECTED(node_cert_data, certificate::create_new(public_key, *node_key, options));
      node_cert = std::make_shared<certificate>(std::move(node_cert_data));
      GET_EXPECTED(node_cert_der, node_cert->der());
      GET_EXPECTED(node_key_der, node_key->der(std::string()));
      CHECK_EXPECTED(t.execute(t1.insert(
          t1.cert = node_cert_der,
          t1.cert_key = node_key_der)));

      //System reserved
      GET_EXPECTED(current_user_folder, persistence::current_user(sp));
      foldername system_reserved(current_user_folder, "reserved");
      CHECK_EXPECTED(system_reserved.create());

      GET_EXPECTED(storage_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
      GET_EXPECTED(storage_public_key, asymmetric_public_key::create(storage_key));

      auto common_storage_cert = cert_control::get_storage_certificate();
      auto common_storage_key = cert_control::get_common_storage_private_key();
      options.name = "!Storage Cert";
      options.country = "RU";
      options.organization = "IVySoft";
      options.ca_certificate = common_storage_cert.get();
      options.ca_certificate_private_key = common_storage_key.get();

      GET_EXPECTED(storage_cert, certificate::create_new(storage_public_key, storage_key, options));

      GET_EXPECTED(node_cert_fingerprint, node_cert->fingerprint(hash::sha256()));
      GET_EXPECTED(storage_key_der, storage_key.der(std::string()));
      GET_EXPECTED(storage_cert_der, storage_cert.der());

      orm::device_config_dbo t2;
      CHECK_EXPECTED(t.execute(t2.insert(
          t2.node_id = node_cert_fingerprint,
          t2.local_path = system_reserved.full_name(),
          t2.owner_id = cert_control::get_storage_certificate()->subject(), //TODO: Need to set default user
          t2.name = "System Reserved",
          t2.reserved_size = 4ULL * 1024 * 1024 * 1024,
          t2.cert = storage_cert_der,
          t2.private_key = storage_key_der)));
    }

      final_tasks.push_back([
        this,
        sp,
        node_cert,
        node_key,
        port,
        udp_transport,
        dev_network]()->async_task<expected<void>> {
        this->udp_transport_task_ = std::make_unique<async_task<expected<void>>>(udp_transport->start(
          sp,
          node_cert,
          node_key,
          port,
          dev_network));

        CHECK_EXPECTED_ASYNC(this->client_.start(
          sp,
          node_cert,
          node_key,
          udp_transport));
        co_return expected<void>();
        });
    return expected<void>();
  }).get());

  while(!final_tasks.empty()) {
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

