/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "vsr_protocol.h"

void vds::server::register_services(service_registrator& registrator)
{
  registrator.add_factory<iserver>([this](bool &)->iserver{
    return iserver(this);
  });
}

void vds::server::start(const service_provider& sp)
{
  this->vsr_server_protocol_.reset(new vsr_protocol::server(sp));
  this->vsr_server_protocol_->start();
}

void vds::server::stop(const service_provider& sp)
{

}

vds::iserver::iserver(vds::server* owner)
: owner_(owner)
{

}
