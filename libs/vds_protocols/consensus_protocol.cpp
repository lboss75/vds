/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "consensus_protocol.h"
#include "consensus_protocol_p.h"

vds::consensus_protocol::server::server(const service_provider & sp, storage_log & storage)
  : impl_(new _server(sp, storage, this))
{
}


vds::consensus_protocol::server::~server()
{
}

void vds::consensus_protocol::server::start()
{
}

void vds::consensus_protocol::server::stop()
{
}

void vds::consensus_protocol::server::register_server(const std::string & certificate_body)
{
}

///////////////////////////////////////////////////////////////////////////////
vds::consensus_protocol::_server::_server(const service_provider & sp, storage_log & storage, server * owner)
  : log_(sp, "Consensus Server"),
  storage_(storage),
  owner_(owner)
{
}

void vds::consensus_protocol::_server::start()
{
}

void vds::consensus_protocol::_server::stop()
{

}
