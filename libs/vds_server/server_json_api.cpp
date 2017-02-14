/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_json_api.h"

vds::server_json_api::server_json_api(
  const service_provider & sp,
  ssl_peer & peer
)
: log_(sp, "Server JSON API"),
  peer_(peer)
{
}

vds::json_value * vds::server_json_api::operator()(json_value * request) const
{
  auto cert = this->peer_.get_peer_certificate();
  this->log_.trace("Certificate subject %s", cert.subject().c_str());
  this->log_.trace("Certificate issuer %s", cert.issuer().c_str());

  auto result = new json_object();
  result->add_property(new json_property("successful", new json_primitive("true")));
  return result;
}
