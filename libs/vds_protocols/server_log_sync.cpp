/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_log_sync.h"
#include "server_log_sync_p.h"

vds::server_log_sync::server_log_sync()
{
}

vds::server_log_sync::~server_log_sync()
{
}

void vds::server_log_sync::register_services(service_registrator&)
{
}

void vds::server_log_sync::start(const service_provider& sp)
{
  this->impl_.reset(new _server_log_sync(sp, this));
  this->impl_->start();
}

void vds::server_log_sync::stop(const service_provider&)
{
  this->impl_->stop();
  this->impl_.reset();
}

////////////////////////////////////////////////
vds::_server_log_sync::_server_log_sync(
  const service_provider & sp,
  server_log_sync * owner)
: sp_(sp), owner_(owner),
  new_local_record(
    [this](const server_log_record & record, const const_data_buffer & signature){
      this->on_new_local_record(record, signature);
    })
{
}

vds::_server_log_sync::~_server_log_sync()
{
}

void vds::_server_log_sync::start()
{
  this->sp_.get<istorage_log>().new_local_record_event() += this->new_local_record;
}

void vds::_server_log_sync::stop()
{
  this->sp_.get<istorage_log>().new_local_record_event() -= this->new_local_record;
}

void vds::_server_log_sync::on_new_local_record(
  const server_log_record & record,
  const const_data_buffer & signature)
{
  //this->connection_manager_.get(this->sp_).broadcast();
  
}


