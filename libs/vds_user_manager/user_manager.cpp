/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
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

vds::user_manager::user_manager()
  : impl_(new _user_manager())
{
}

vds::member_user vds::user_manager::create_root_user(
  transaction_block & log,
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  return this->impl_->create_root_user(log, user_name, user_password, private_key);
}

vds::user_channel vds::user_manager::create_channel(transaction_block &log, const vds::member_user &owner,
                                                    const vds::asymmetric_private_key &owner_user_private_key,
                                                    const std::string &name,
                                                    const asymmetric_private_key &read_private_key,
                                                    const asymmetric_private_key &write_private_key) {
  return this->impl_->create_channel(log, owner, owner_user_private_key, name, read_private_key, write_private_key);
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

vds::const_data_buffer vds::user_manager::reset(const service_provider &sp, database_transaction &t, const std::string &root_user_name,
                                                const std::string &root_password, const asymmetric_private_key &root_private_key,
                                                const std::string &device_name, int port) {

  transaction_block log;

  auto user = this->create_root_user(
      log,
      root_user_name,
      root_password,
      root_private_key);

  certificate_dbo t1;
  t.execute(t1.insert(
      t1.id = user.id(),
      t1.cert = user.user_certificate().der()));

  auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  auto read_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  auto write_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  auto common_channel = this->create_channel(
      log,
      user,
      private_key,
      "Common chanel",
      read_private_key,
      write_private_key);

  certificate_private_key_dbo t2;
  t.execute(t2.insert(
      t2.id = cert_control::get_id(common_channel.write_cert()),
      t2.body = write_private_key.der(std::string())));

  auto device_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  this->lock_to_device(sp, t, log, user, root_user_name, root_password,
                       root_private_key, device_name, device_key, port);

  return log.sign(
      cert_control::get_id(common_channel.write_cert()),
      common_channel.write_cert(),
      user.id(),
      root_private_key);
}

vds::member_user
vds::user_manager::lock_to_device(const vds::service_provider &sp, vds::database_transaction &t, transaction_block &log,
                                  const member_user &user, const std::string &user_name,
                                  const std::string &user_password, const asymmetric_private_key &user_private_key,
                                  const std::string &device_name,
                                  const asymmetric_private_key &device_private_key,
                                  int port) {

  auto device_user = user.create_device_user(log, user_private_key, device_private_key, device_name);

  auto config_id = guid::new_guid();
  run_configuration_dbo t3;
  t.execute(
      t3.insert(
          t3.id = config_id,
          t3.cert_id = device_user.id(),
          t3.port = port));

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


////////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager()
{
}

vds::member_user vds::_user_manager::create_root_user(
  transaction_block & log,
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  return _member_user::create_root(log, user_name, user_password, private_key);
}

vds::user_channel vds::_user_manager::create_channel(transaction_block &log, const vds::member_user &owner,
                                                     const vds::asymmetric_private_key &owner_user_private_key,
                                                     const std::string &name,
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

  log.add(create_channel_transaction(
      id,
      owner.id()));

  log.add(channel_add_message_transaction(
      owner.id(),
      owner.user_certificate(),
      owner_user_private_key,
      channel_add_message_transaction::create_channel(
          id,
          read_id, read_cert, read_private_key,
          write_id, write_cert, write_private_key)));
  return user_channel(id, read_cert, write_cert);
}