/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_log_sync.h"
#include "server_log_sync_p.h"
#include "messages.h"
#include "server_database.h"
#include "principal_manager_p.h"

vds::server_log_sync::server_log_sync()
: impl_(new _server_log_sync(this))
{
}

vds::server_log_sync::~server_log_sync()
{
}

void vds::server_log_sync::register_services(service_registrator& registrator)
{
  registrator.add_service<_server_log_sync>(this->impl_.get());
}

void vds::server_log_sync::start(const service_provider& sp)
{
  this->impl_->start(sp);
}

void vds::server_log_sync::stop(const service_provider& sp)
{
  this->impl_->stop(sp);
}

////////////////////////////////////////////////
vds::_server_log_sync::_server_log_sync(
  server_log_sync * owner)
: owner_(owner)
{
}

vds::_server_log_sync::~_server_log_sync()
{
}

void vds::_server_log_sync::start(const service_provider & sp)
{
  this->timer_.start(
    sp,
    std::chrono::seconds(5), [this, sp]() {
      return this->process_timer_jobs(sp);
  });
}

void vds::_server_log_sync::stop(const service_provider & sp)
{
  this->timer_.stop(sp);
}

void vds::_server_log_sync::on_new_local_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  sp.get<logger>()->debug(sp, "Broadcast %s", record.id().str().c_str());
  sp.get<iconnection_manager>()->broadcast(sp, server_log_record_broadcast(record, signature));
}

void vds::_server_log_sync::on_record_broadcast(
  const service_provider & sp,
  const server_log_record_broadcast & message)
{
  if((*sp.get<principal_manager>())->save_record(
    sp,
    message.record(),
    message.signature())){
    sp.get<logger>()->debug(sp, "Got %s", message.record().id().str().c_str());

    sp.get<iconnection_manager>()->broadcast(sp, server_log_record_broadcast(message.record(), message.signature()));
    this->require_unknown_records(sp);
  }
}

void vds::_server_log_sync::on_server_log_get_records_broadcast(
  const service_provider & sp,
  const connection_session & session,
  const server_log_get_records_broadcast & message)
{
  for (auto p : message.unknown_records()) {
    principal_log_record record;
    const_data_buffer signature;
      if((*sp.get<principal_manager>())->get_record(sp, p, record, signature)) {
        sp.get<logger>()->debug(sp, "Provided %s", record.id().str().c_str());
        sp.get<iconnection_manager>()->send_to(sp, session, server_log_record_broadcast(record, signature));
      }
  }
}

void vds::_server_log_sync::require_unknown_records(
  const service_provider & sp)
{
  std::list<principal_log_record::record_id> unknown_records;
  (*sp.get<principal_manager>())->get_unknown_records(sp, unknown_records);

  if (!unknown_records.empty()) {

    for (auto& p : unknown_records) {
      sp.get<logger>()->debug(sp, "Require %s", p.str().c_str());
    }

    sp.get<iconnection_manager>()->broadcast(sp, server_log_get_records_broadcast(unknown_records));
  }
}

bool vds::_server_log_sync::process_timer_jobs(const service_provider & sp)
{
  this->require_unknown_records(sp);
  return true;
}

void vds::_server_log_sync::ensure_record_exists(const service_provider & sp, const principal_log_record::record_id & record_id)
{
  if (principal_manager::principal_log_state::not_found == (*sp.get<principal_manager>())->get_record_state(sp, record_id)) {
    std::list<principal_log_record::record_id> unknown_records;
    unknown_records.push_back(record_id);
    sp.get<iconnection_manager>()->broadcast(sp, server_log_get_records_broadcast(unknown_records));
  }
}

//////////////////////////////////////////////////
const char vds::_server_log_sync::server_log_record_broadcast::message_type[] = "server log";
const uint32_t vds::_server_log_sync::server_log_record_broadcast::message_type_id = (uint32_t)message_identification::server_log_record_broadcast_message_id;

vds::_server_log_sync::server_log_record_broadcast::server_log_record_broadcast(
  const principal_log_record & record,
  const const_data_buffer & signature)
: record_(record),
  signature_(signature)
{
}

vds::_server_log_sync::server_log_record_broadcast::server_log_record_broadcast(
  const service_provider & sp,
  const const_data_buffer & data)
{
  binary_deserializer s(data);
  this->record_.deserialize(sp, s);
  s >> this->signature_;
}

void vds::_server_log_sync::server_log_record_broadcast::serialize(binary_serializer & b) const
{
  this->record_.serialize(b);
  b << this->signature_;
}

std::shared_ptr<vds::json_value> vds::_server_log_sync::server_log_record_broadcast::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("r", this->record_.serialize(false));
  result->add_property("s", this->signature_);

  return std::shared_ptr<json_value>(result.release());
}

//////////////////////////////////////////////////
const char vds::_server_log_sync::server_log_get_records_broadcast::message_type[] = "server log request";
const uint32_t vds::_server_log_sync::server_log_get_records_broadcast::message_type_id = (uint32_t)message_identification::server_log_get_records_broadcast_message_id;

vds::_server_log_sync::server_log_get_records_broadcast::server_log_get_records_broadcast(
  const std::list<principal_log_record::record_id> & unknown_records)
: unknown_records_(unknown_records)
{
}

vds::_server_log_sync::server_log_get_records_broadcast::server_log_get_records_broadcast(const const_data_buffer & data)
{
  binary_deserializer s(data);
  auto count = s.read_number();
  for (decltype(count) i = 0; i < count; ++i) {
    principal_log_record::record_id item;
    s >> item;
    this->unknown_records_.push_back(item);
  }
}

void vds::_server_log_sync::server_log_get_records_broadcast::serialize(binary_serializer & b) const
{
  b.write_number(this->unknown_records_.size());
  for (auto & p : this->unknown_records_) {
    b << p;
  }
}

std::shared_ptr<vds::json_value> vds::_server_log_sync::server_log_get_records_broadcast::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);

  return std::shared_ptr<json_value>(result.release());
}

