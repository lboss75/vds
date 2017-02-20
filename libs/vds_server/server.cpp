/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "vsr_protocol.h"
#include "node_manager.h"

void vds::server::register_services(service_registrator& registrator)
{
  registrator.add_factory<iserver>([this](bool &)->iserver{
    return iserver(this);
  });
  
  registrator.add_factory<vsr_protocol::iserver>([this](bool &)->vsr_protocol::iserver{
    return vsr_protocol::iserver(this->vsr_server_protocol_.get());
  });
}

void vds::server::start(const service_provider& sp)
{
  this->vsr_server_protocol_.reset(new vsr_protocol::server(sp));
  this->vsr_server_protocol_->start();

  this->node_manager_.reset(new node_manager(sp));
}

void vds::server::stop(const service_provider& sp)
{

}

vds::iserver::iserver(vds::server* owner)
: owner_(owner)
{

}
