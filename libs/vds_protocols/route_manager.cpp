/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "route_manager.h"
#include "route_manager_p.h"
#include "connection_manager_p.h"
#include "storage_log.h"

vds::route_manager::route_manager()
  : impl_(new _route_manager())
{
}

vds::route_manager::~route_manager()
{
  delete this->impl_;
}

void vds::route_manager::add_route(
  const service_provider& sp,
  const guid& source_server_id,
  const guid& target_server_id,
  const std::string& address)
{

}

void vds::route_manager::get_routes(
  const service_provider& sp,
  const guid& target_server_id,
  const std::function<bool(size_t metric, std::list<network_route> & routes)> & callback)
{

}

void vds::route_manager::send_to(
  const service_provider & sp,
  const guid & server_id,
  uint32_t message_type_id,
  const const_data_buffer & message_data)
{
  this->impl_->send_to(sp, server_id, message_type_id, message_data);
}

//////////////////////////////////
vds::_route_manager::_route_manager()
{
}

vds::_route_manager::~_route_manager()
{
}

void vds::_route_manager::send_to(
  const service_provider & sp,
  const guid & server_id,
  uint32_t message_type_id,
  const const_data_buffer & message_data)
{
  bool result = false;
  auto con_man = sp.get<iconnection_manager>();
  (*con_man)->enum_sessions(
    [&result, sp, con_man, server_id, message_type_id, message_data](connection_session & session)->bool {
    if (server_id == session.server_id()) {
      con_man->send_to(sp, session, route_message(
        guid::new_guid(),
        server_id,
        message_type_id,
        message_data,
        session.server_id(),
        session.address()));
      result = true;
      return false;
    }

    return true;
  });

  if (result) {
    return;
  }

  (*con_man)->enum_sessions(
    [&result, sp, con_man, server_id, message_type_id, message_data](connection_session & session)->bool {
    con_man->send_to(
      sp,
      session,
      route_message(
        guid::new_guid(),
        server_id,
        message_type_id,
        message_data,
        session.server_id(),
        session.address()));
    return true;
  });
}


void vds::_route_manager::on_session_started(
  const service_provider& sp,
  const guid & source_server_id,
  const guid & target_server_id,
  const std::string & address)
{
}

void vds::_route_manager::on_route_message(
  _connection_manager * con_man,
  const service_provider & sp,
  database_transaction & t,
  const connection_session & session,
  const route_message & message)
{
  this->processed_route_message_mutex_.lock();
  if (this->processed_route_message_.end() != this->processed_route_message_.find(message.message_id())) {
    this->processed_route_message_mutex_.unlock();
    return;
  }

  this->processed_route_message_.set(message.message_id(), message.message_id());
  this->processed_route_message_mutex_.unlock();

  if (sp.get<istorage_log>()->current_server_id() == message.target_server_id()) {
    //TODO: add signarute to the message and validate it
    con_man->server_to_server_api_.process_message(sp, t, con_man, session, message.msg_type_id(), message.message_data());
  }
  else {
    con_man->possible_connections(sp, message.trace_route());

    bool result = false;
    auto con_man = sp.get<iconnection_manager>();
    (*con_man)->enum_sessions(
      [&result, sp, con_man, &message](connection_session & session)->bool {
      if (message.target_server_id() == session.server_id()) {
        con_man->send_to(sp, session, route_message(message, session.server_id(), session.address()));
        result = true;
        return false;
      }

      return true;
    });

    if (result) {
      return;
    }

    (*con_man)->enum_sessions(
      [&result, sp, con_man, &message](connection_session & session)->bool {
      con_man->send_to(sp, session, route_message(message, session.server_id(), session.address()));
      return true;
    });
  }
}

////////////////////////////////////////////
vds::route_message::route_message(const const_data_buffer & binary_form)
{
  binary_deserializer b(binary_form);
  b >> this->target_server_id_ >> this->message_type_id_ >> this->message_data_;
}

void vds::route_message::serialize(vds::binary_serializer& b) const
{
  b << this->target_server_id_ << this->message_type_id_ << this->message_data_;
}
