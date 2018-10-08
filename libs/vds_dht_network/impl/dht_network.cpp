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

void vds::dht::network::service::register_services(service_registrator& registrator) {
  registrator.add_service<client>(&this->client_);
}

void vds::dht::network::service::start(
  const service_provider * sp,
  const std::shared_ptr<iudp_transport> & udp_transport,
  const uint16_t port) {
  sp->get<db_model>()->async_transaction([this, sp, udp_transport, port](database_transaction& t) {
    std::shared_ptr<certificate> node_cert;
    std::shared_ptr<asymmetric_private_key> node_key;

      orm::current_config_dbo t1;
      auto st = t.get_reader(t1.select(t1.cert, t1.cert_key));
      if (st.execute()) {
        node_cert = std::make_shared<certificate>(certificate::parse_der(t1.cert.get(st)));
        node_key = std::make_shared<asymmetric_private_key>(asymmetric_private_key::parse_der(t1.cert_key.get(st), std::string()));
      }
      else {
        node_key = std::make_shared<asymmetric_private_key>(asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
      asymmetric_public_key public_key(*node_key);

      certificate::create_options options;
      options.name = "Node Cert";
      options.country = "RU";
      options.organization = "IVySoft";
      node_cert = std::make_shared<certificate>(certificate::create_new(public_key, *node_key, options));
      t.execute(t1.insert(
          t1.cert = node_cert->der(),
          t1.cert_key = node_key->der(std::string())));

      //System reserved
      foldername system_reserved(foldername(persistence::current_user(sp), ".vds"), "reserved");
      system_reserved.create();

      orm::device_config_dbo t2;
      t.execute(t2.insert(
          t2.node_id = node_cert->fingerprint(hash::sha256()),
          t2.local_path = system_reserved.full_name(),
          t2.owner_id = "root_user", //TODO: Need to set default user
          t2.name = "System Reserved",
          t2.reserved_size = 4ULL * 1024 * 1024 * 1024));
    }

    udp_transport->start(
      sp,
      node_cert,
      node_key, 
      port);

    this->client_.start(
        sp,
        node_cert,
        node_key,
        udp_transport);
    return true;
  }).get();
}

void vds::dht::network::service::stop() {
  this->client_.stop();
}

std::future<void> vds::dht::network::service::prepare_to_stop() {
  co_return;
}

