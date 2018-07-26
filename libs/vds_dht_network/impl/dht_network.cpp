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

void vds::dht::network::service::start(const service_provider& parent_scope, uint16_t port) {
  auto sp = parent_scope.create_scope(__FUNCTION__);
  mt_service::enable_async(sp);
  sp.get<db_model>()->async_transaction(sp, [this, sp, port](database_transaction & t) {
    auto node_cert = prepare_to_start(sp, t);
    this->client_.start(sp, node_cert.fingerprint(hash::sha256()), port);
    return true;
  }).wait();
}

void vds::dht::network::service::stop(const service_provider& sp) {
  this->client_.stop(sp);
}

vds::async_task<> vds::dht::network::service::prepare_to_stop(const service_provider& sp) {
  return vds::async_task<>::empty();
}

vds::certificate vds::dht::network::service::prepare_to_start(const service_provider& sp, database_transaction & t) {
    orm::current_config_dbo t1;
    auto st = t.get_reader(t1.select(t1.cert, t1.cert_key));
    if (st.execute()) {
      return certificate::parse_der(t1.cert.get(st));
    }
    else {
      auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
      asymmetric_public_key public_key(private_key);

      certificate::create_options options;
      options.name = "Node Cert";
      options.country = "RU";
      options.organization = "IVySoft";
      auto result = certificate::create_new(public_key, private_key, options);
      t.execute(t1.insert(
        t1.cert = result.der(),
        t1.cert_key = private_key.der(std::string())));

      //System reserved
      foldername system_reserved(foldername(persistence::current_user(sp), ".vds"), "reserved");
      system_reserved.create();

      orm::device_config_dbo t2;
      t.execute(t2.insert(
        t2.node_id = result.fingerprint(hash::sha256()),
        t2.local_path = system_reserved.full_name(),
        t2.owner_id = "root_user", //TODO: Need to set default user
        t2.name = "System Reserved",
        t2.reserved_size = 4ULL * 1024 * 1024 * 1024));

      return result;
    }
}
