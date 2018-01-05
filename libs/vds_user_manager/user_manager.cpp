/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <transactions/channel_add_reader_transaction.h>
#include "stdafx.h"
#include "user_manager.h"
#include "member_user.h"
#include "user_manager_storage.h"
#include "private/user_manager_p.h"
#include "private/member_user_p.h"
#include "database_orm.h"
#include "certificate_dbo.h"
#include "user_dbo.h"
#include "transactions/root_user_transaction.h"
#include "transactions/create_channel_transaction.h"
#include "transactions/channel_add_member_transaction.h"
#include "transactions/channel_add_message_transaction.h"
#include "private/cert_control_p.h"
#include "transaction_context.h"
#include "channel_dbo.h"
#include "channel_admin_dbo.h"
#include "cert_control.h"
#include "channel_message_dbo.h"
#include "certificate_private_key_dbo.h"
#include "transactions/device_user_add_transaction.h"
#include "run_configuration_dbo.h"
#include "channel_keys_dbo.h"
#include "vds_exceptions.h"
#include "transactions/channel_create_transaction.h"

vds::user_manager::user_manager()
  : impl_(new _user_manager())
{
}

vds::user_channel vds::user_manager::create_channel(transaction_block &log, const guid &common_channel_id, const vds::member_user &owner,
                                                    const vds::asymmetric_private_key &owner_user_private_key, const std::string &name,
                                                    const asymmetric_private_key &read_private_key,
                                                    const asymmetric_private_key &write_private_key) {
  return this->impl_->create_channel(log, common_channel_id, owner, owner_user_private_key, name, read_private_key,
                                     write_private_key);
}

void vds::user_manager::apply_transaction_record(
    const service_provider &sp,
    database_transaction & t,
    uint8_t message_id,
    binary_deserializer & s) {

  switch(message_id){
    case root_user_transaction::message_id:
    {
      root_user_transaction record(s);

      user_dbo usr_dbo;
      t.execute(
          usr_dbo.insert(
              usr_dbo.id = record.id(),
              usr_dbo.login = record.user_name(),
              usr_dbo.password_hash = record.password_hash(),
              usr_dbo.private_key = record.user_private_key()));

      break;
    }

    case create_channel_transaction::message_id:
    {
      create_channel_transaction record(s);
      if(record.owner_id() != sp.get_property<transaction_context>(
          service_provider::property_scope::local_scope)->author_id()){
        throw std::runtime_error("Unable to create channels for other users");
      }

      channel_dbo t1;
      t.execute(
          t1.insert(t1.id = record.id(), t1.channel_type = (uint8_t)channel_dbo::channel_type_t::simple));

      channel_admin_dbo t2;
      t.execute(
          t2.insert(t2.id = record.id(), t2.member_id = record.owner_id()));

      break;
    }

    case channel_add_message_transaction::message_id:
    {
      channel_add_message_transaction record(s);

      channel_message_dbo t1;
      t.execute(t1.insert(
          t1.channel_id = record.channel_id(),
          t1.cert_id = record.cert_id(),
          t1.message = record.message()));

      break;
    }

    default:
      throw std::runtime_error("Invalid record ID");
  }

}

void vds::user_manager::reset(
    const service_provider &sp,
    database_transaction &t,
    const std::string &root_user_name,
    const std::string &root_password,
    const asymmetric_private_key &root_private_key,
    const std::string &device_name,
    int port) {

  //Create root user
  auto root_user_id = guid::new_guid();
  auto root_user_cert = _cert_control::create_root(
      root_user_id,
      "User " + root_user_name,
      root_private_key);

  //Create common channel
  guid common_channel_id = guid::new_guid();
  auto read_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto read_id = vds::guid::new_guid();
  auto read_cert = vds::_cert_control::create(
      read_id,
      "Read Member Certificate " + read_id.str(),
      read_private_key,
      root_user_id,
      root_user_cert,
      root_private_key);

  auto write_id = vds::guid::new_guid();
  auto write_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto write_cert = vds::_cert_control::create(
      write_id,
      "Write Member Certificate " + write_id.str(),
      write_private_key,
      root_user_id,
      root_user_cert,
      root_private_key);

  transactions::transaction_block playback;
  auto common_channel_playback = playback
      .create_channel(common_channel_id, read_cert, write_cert, write_private_key);

  common_channel_playback.add(
      root_user_transaction(
          root_user_id,
          root_user_cert,
          root_user_name,
          root_private_key.der(root_password),
          hash::signature(hash::sha256(), root_password.c_str(), root_password.length())));

  //Create common channel
  this->create_channel(
      log,
      common_channel_id,
      "Common",
      common_channel_id,
      root_user_id,
      root_user_cert,
      root_private_key);

  auto read_private_key = asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto read_id = guid::new_guid();
  auto read_cert = _cert_control::create(
      read_id,
      "Read Member Certificate " + read_id.str(),
      read_private_key,
      root_user_id,
      cert,
      root_private_key);

  common_channel_playback.add(
      common_channel_id,
      transactions::channel_create_transaction(
          transactions::channel_create_transaction::channel_type::user,
          root_user_id,
          read_cert,
          read_private_key));

  common_channel_playback.add(
      root_user_id,
      transactions::channel_add_reader_transaction(
          root_user_id,
          cert,
          read_private_key));

  log.pack();

  transaction_log::apply(sp, t, log);

  //Lock to device
  auto user = this->by_login(t, root_user_name);
  auto device_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  auto device_user = this->lock_to_device(
      sp, t, user, root_user_name, root_password, root_private_key,
      device_name, device_key, common_channel_id, port);

  log.add(
      device_user.id(),
      transactions::channel_add_reader_transaction(
          root_user_id,
          cert,
          read_private_key));
}

vds::user_channel
vds::user_manager::create_channel(
    transactions::transaction_block &log,
    const vds::guid &channel_id,
    const std::string & name,
    const vds::guid &common_channel_id,
    const vds::guid &owner_id,
    const certificate &owner_cert,
    const asymmetric_private_key &owner_private_key) const {
  auto read_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto read_id = vds::guid::new_guid();
  auto read_cert = vds::_cert_control::create(
      read_id,
      "Read Member Certificate " + read_id.str(),
      read_private_key,
      owner_id,
      owner_cert,
      owner_private_key);

  auto write_id = vds::guid::new_guid();
  auto write_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto write_cert = vds::_cert_control::create(
      write_id,
      "Write Member Certificate " + write_id.str(),
      write_private_key,
      owner_id,
      owner_cert,
      owner_private_key);

  log.add(
      common_channel_id,
      transactions::channel_create_transaction(
          channel_id,
          name,
          read_cert, read_private_key,
          write_cert, write_private_key));
}


vds::member_user
vds::user_manager::lock_to_device(const vds::service_provider &sp, vds::database_transaction &t,
                                  const member_user &user, const std::string &user_name,
                                  const std::string &user_password,
                                  const asymmetric_private_key &user_private_key,
                                  const std::string &device_name,
                                  const asymmetric_private_key &device_private_key,
                                  const guid &common_channel_id, int port) {

  auto device_user = user.create_device_user(
      user_private_key,
      device_private_key,
      device_name);

  auto config_id = guid::new_guid();
  run_configuration_dbo t3;
  t.execute(
      t3.insert(
          t3.id = config_id,
          t3.cert_id = device_user.id(),
          t3.port = port,
          t3.common_channel_id = common_channel_id));

  certificate_dbo t4;
  t.execute(
      t4.insert(
          t4.id = device_user.id(),
          t4.cert = device_user.user_certificate().der()));

  certificate_private_key_dbo t5;
  t.execute(
      t5.insert(
          t5.id = device_user.id(),
          t5.body = device_private_key.der(std::string())));

  return device_user;
}

vds::member_user vds::user_manager::by_login(
    vds::database_transaction &t,
    const std::string &login) {
  return vds::member_user::by_login(t, login);
}

vds::member_user vds::user_manager::import_user(const certificate &user_cert) {
  return vds::member_user::import_user(user_cert);
}

vds::member_user vds::user_manager::get_current_device(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    asymmetric_private_key &device_private_key) {

  run_configuration_dbo t1;
  auto st = t.get_reader(t1.select(t1.cert_id));
  if(!st.execute()){
    throw std::runtime_error("Unable to get current configuration");
  }
  auto user_id = t1.cert_id.get(st);

  certificate_private_key_dbo t2;
  st = t.get_reader(t2.select(t2.body).where(t2.id == user_id));
  if(!st.execute()){
    throw std::runtime_error("Unable to load user private key");
  }
  device_private_key = asymmetric_private_key::parse_der(t2.body.get(st), std::string());

  certificate_dbo t3;
  st = t.get_reader(t3.select(t3.cert).where(t3.id == user_id));
  if(!st.execute()){
    throw std::runtime_error("Unable to load user certificate");
  }

  return member_user(
      new _member_user(
          user_id,
          certificate::parse_der(t3.cert.get(st))));
}

vds::asymmetric_private_key
vds::user_manager::get_channel_write_key(
    const service_provider &sp,
    database_transaction &t,
    const user_channel &channel,
    const guid &user_id) {
  channel_keys_dbo t1;
  auto st = t.get_reader(
      t1.select(t1.write_key)
          .where(
              t1.channel_id == channel.id()
              && t1.user_id == user_id
              && t1.cert_id == cert_control::get_id(channel.write_cert())));
  if(!st.execute()){
    throw vds_exceptions::not_found();
  }

  return asymmetric_private_key::parse_der(
      t1.write_key.get(st), std::string());
}

vds::user_channel vds::user_manager::get_common_channel(vds::database_transaction &t) const {
  run_configuration_dbo t1;
  auto st = t.get_reader(t1.select(t1.common_channel_id));
  if(!st.execute()){
    throw std::runtime_error("Unable to load common channel id");
  }

  return this->get_channel(t, t1.common_channel_id.get(st));
}

vds::user_channel vds::user_manager::get_channel(
    vds::database_transaction &t,
    const vds::guid &channel_id) const {
  channel_dbo t1;
  certificate_dbo t2;
  certificate_dbo t3;
  auto st = t.get_reader(t1.select(t2.cert, t3.cert)
                    .left_join(t2, t2.id == t1.read_cert)
                    .left_join(t3, t3.id == t1.write_cert)
                   .where(t1.id == channel_id));
  if(!st.execute()){
    throw vds_exceptions::not_found();
  }
  return vds::user_channel(
      channel_id,
      (t2.cert.is_null(st) ? certificate() : certificate::parse_der(t2.cert.get(st))),
      (t3.cert.is_null(st) ? certificate() : certificate::parse_der(t3.cert.get(st))));
}

////////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager()
{
}

vds::user_channel vds::_user_manager::create_channel(transaction_block &log, const guid &common_channel_id, const vds::member_user &owner,
                                                     const vds::asymmetric_private_key &owner_user_private_key, const std::string &name,
                                                     const asymmetric_private_key &read_private_key,
                                                     const asymmetric_private_key &write_private_key)
{
  auto read_id = guid::new_guid();
  auto read_cert = _cert_control::create(
      read_id,
      "Read Member Certificate " + read_id.str(),
      read_private_key,
      owner.id(),
      owner.user_certificate(),
      owner_user_private_key);

  auto write_id = guid::new_guid();
  auto write_cert = _cert_control::create(
      write_id,
      "Write Member Certificate " + write_id.str(),
      write_private_key,
      owner.id(),
      owner.user_certificate(),
      owner_user_private_key);

  auto id = guid::new_guid();

  log.add(
      common_channel_id,
      create_channel_transaction(
        id,
        owner.id()));

  log.add(
      owner.id(),
      channel_add_message_transaction::create_channel(
          id,
          read_id, read_cert, read_private_key,
          write_id, write_cert, write_private_key));
  return user_channel(id, read_cert, write_cert);
}