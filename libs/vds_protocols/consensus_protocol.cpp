/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "consensus_protocol.h"
#include "consensus_protocol_p.h"
#include "node.h"
#include "storage_service.h"

vds::consensus_protocol::server::server(
  const service_provider & sp,
  storage_log & storage,
  certificate & certificate,
  asymmetric_private_key & private_key)
  : impl_(new _server(sp, storage, this, certificate, private_key))
{
}


vds::consensus_protocol::server::~server()
{
}

void vds::consensus_protocol::server::start()
{
  this->impl_->start();
}

void vds::consensus_protocol::server::stop()
{
  this->impl_->stop();
}

void vds::consensus_protocol::server::register_server(const std::string & certificate_body)
{
  this->impl_->register_server(certificate_body);
}

///////////////////////////////////////////////////////////////////////////////
vds::consensus_protocol::_server::_server(
  const service_provider & sp,
  storage_log & storage,
  server * owner,
  certificate & certificate,
  asymmetric_private_key & private_key)
  : sp_(sp),
  log_(sp, "Consensus Server"),
  storage_(storage),
  owner_(owner),
  certificate_(certificate),
  private_key_(private_key)
{
}

void vds::consensus_protocol::_server::start()
{
  storage_cursor<node> node_reader(this->sp_.get<istorage>());
  while (node_reader.read()) {
    this->nodes_[node_reader.current().id()] = { };
  }
}

void vds::consensus_protocol::_server::stop()
{

}

void vds::consensus_protocol::_server::register_server(const std::string & certificate_body)
{
  //server_log_new_server(certificate_body).serialize()->str();
    
}
