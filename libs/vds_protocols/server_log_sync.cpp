/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_log_sync.h"
#include "server_log_sync_p.h"
#include "messages.h"

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
  new_local_record_(
    [this](const server_log_record & record, const const_data_buffer & signature){
      this->on_new_local_record(record, signature);
    }),
  record_broadcast_(
    [this](const const_data_buffer & data) {
      this->on_record_broadcast(server_log_record_broadcast(data));
    })
{
}

vds::_server_log_sync::~_server_log_sync()
{
}

void vds::_server_log_sync::start()
{
  this->sp_.get<istorage_log>().new_local_record_event() += this->new_local_record_;
  this->sp_.get<iconnection_manager>().incoming_message(
    (uint32_t)message_identification::server_log_record_broadcast_message_id) += this->record_broadcast_;
}

void vds::_server_log_sync::stop()
{
  this->sp_.get<istorage_log>().new_local_record_event() -= this->new_local_record_;
}

void vds::_server_log_sync::on_new_local_record(
  const server_log_record & record,
  const const_data_buffer & signature)
{
  this->connection_manager_.get(this->sp_)
    .broadcast(server_log_record_broadcast(record, signature));
}

void vds::_server_log_sync::on_record_broadcast(const server_log_record_broadcast & message)
{
  if(this->sp_.get<istorage_log>().apply_record(message.record(), message.signature())){
    this->connection_manager_.get(this->sp_)
      .broadcast(server_log_record_broadcast(message.record(), message.signature()));
  }
}

//////////////////////////////////////////////////
const char vds::_server_log_sync::server_log_record_broadcast::message_type[] = "server log";
const uint32_t vds::_server_log_sync::server_log_record_broadcast::message_type_id = (uint32_t)message_identification::server_log_record_broadcast_message_id;

vds::_server_log_sync::server_log_record_broadcast::server_log_record_broadcast(
  const server_log_record & record,
  const const_data_buffer & signature)
: record_(record),
  signature_(signature)
{
}

vds::_server_log_sync::server_log_record_broadcast::server_log_record_broadcast(const const_data_buffer & data)
{
  binary_deserializer s(data);
  s >> this->record_ >> this->signature_;
}

void vds::_server_log_sync::server_log_record_broadcast::serialize(binary_serializer & b) const
{
  this->record_.serialize(b);
  this->signature_.serialize(b);
}

std::unique_ptr<vds::json_value> vds::_server_log_sync::server_log_record_broadcast::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("r", this->record_.serialize(false));
  result->add_property("s", this->signature_);

  return std::unique_ptr<json_value>(result.release());
}


