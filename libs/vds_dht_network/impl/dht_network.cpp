/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "dht_network.h"
#include "private/udp_transport.h"

void vds::dht::network::service::register_services(service_registrator& registrator) {
}

void vds::dht::network::service::start(const service_provider& sp, uint16_t port) {
  this->udp_transport_ = std::make_shared<udp_transport>();
  this->udp_transport_->start(sp, port);
}

void vds::dht::network::service::stop(const service_provider&) {
}

vds::async_task<> vds::dht::network::service::prepare_to_stop(const service_provider& sp) {
  return vds::async_task<>::empty();
}
