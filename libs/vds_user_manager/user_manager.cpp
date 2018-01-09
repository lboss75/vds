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
#include "channel_keys_dbo.h"
#include "vds_exceptions.h"
#include "transactions/channel_create_transaction.h"
#include "transactions/channel_add_reader_transaction.h"
#include "transactions/channel_add_writer_transaction.h"
#include "channel_member_dbo.h"
#include "transactions/user_channel_create_transaction.h"

vds::user_manager::user_manager()
  : impl_(new _user_manager())
{
}

void vds::user_manager::apply_transaction_record(
    const service_provider &sp,
    database_transaction & t,
    uint8_t message_id,
    binary_deserializer & s) {

  switch(message_id){
    case transactions::root_user_transaction::message_id:
    {
      transactions::root_user_transaction record(s);

      user_dbo usr_dbo;
      t.execute(
          usr_dbo.insert(
              usr_dbo.id = record.id(),
              usr_dbo.login = record.user_name(),
              usr_dbo.password_hash = record.password_hash(),
              usr_dbo.private_key = record.user_private_key()));

      break;
    }

    case transactions::create_channel_transaction::message_id:
    {
      transactions::create_channel_transaction record(s);
      if(record.owner_id() != sp.get_property<transaction_context>(
          service_provider::property_scope::local_scope)->author_id()){
        throw std::runtime_error("Unable to create channels for other users");
      }

      orm::channel_dbo t1;
      t.execute(
          t1.insert(
              t1.id = record.id(),
              t1.channel_type = (uint8_t)orm::channel_dbo::channel_type_t::simple));

      channel_admin_dbo t2;
      t.execute(
          t2.insert(t2.id = record.id(), t2.member_id = record.owner_id()));

      break;
    }

    case transactions::channel_add_message_transaction::message_id:
    {
      transactions::channel_add_message_transaction record(s);

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

  guid common_channel_id = guid::new_guid();
  transactions::transaction_block playback;
  //Create root user
  auto root_user = this->create_root_user(playback, t, common_channel_id, root_user_name, root_password,
                                          root_private_key);

  //Create common channel
  auto common_channel = this->create_channel(
      playback, t,
      common_channel_id,
      common_channel_id,
      "Common channel",
      root_user.id(),
      root_user.user_certificate(),
      root_private_key);

  auto user_channel = this->create_user_channel(
      playback, t,
      common_channel_id,
      root_user.id(),
      root_user.user_certificate(),
      root_private_key);

  //Lock to device
  auto user = this->by_login(t, root_user_name);
  auto device_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  auto device_user = this->lock_to_device(
      sp, t, user, root_user_name, root_password, root_private_key,
      device_name, device_key, common_channel_id, port);

  this->allow_read(
      t,
      device_user,
      common_channel.id(),
      common_channel.read_cert(),
      this->get_private_key(
          t,
          cert_control::get_id(common_channel.read_cert()),
          cert_control::get_id(root_user.user_certificate()),
          root_private_key));

  playback.add(
      common_channel_id,
      transactions::device_user_add_transaction(
          device_user.id(),
          device_user.user_certificate()));

  playback.save(
      sp,
      t,
      common_channel.read_cert(),
      root_user.user_certificate(),
      root_private_key);
}

vds::user_channel
vds::user_manager::create_channel(
    transactions::transaction_block &log,
    database_transaction &t,
    const vds::guid &common_channel_id,
    const vds::guid &channel_id,
    const std::string &name,
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

  certificate_dbo t1;
  t.execute(t1.insert(
      t1.id = read_id,
      t1.parent = cert_control::get_id(owner_cert),
      t1.cert = read_cert.der()
  ));

  orm::certificate_private_key_dbo t2;
  t.execute(t2.insert(
      t2.id = read_id,
      t2.owner_id = cert_control::get_id(owner_cert),
      t2.body = owner_cert.public_key().encrypt(read_private_key.der(std::string()))
  ));

  auto write_id = vds::guid::new_guid();
  auto write_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto write_cert = vds::_cert_control::create(
      write_id,
      "Write Member Certificate " + write_id.str(),
      write_private_key,
      owner_id,
      owner_cert,
      owner_private_key);

  t.execute(t1.insert(
      t1.id = write_id,
      t1.parent = cert_control::get_id(owner_cert),
      t1.cert = write_cert.der()
  ));
  t.execute(t2.insert(
      t2.id = write_id,
      t2.owner_id = cert_control::get_id(owner_cert),
      t2.body = owner_cert.public_key().encrypt(write_private_key.der(std::string()))
  ));

  orm::channel_dbo t3;
  t.execute(t3.insert(
      t3.id = channel_id,
      t3.channel_type = (uint8_t)orm::channel_dbo::channel_type_t::simple,
      t3.name = name,
      t3.read_cert = read_id,
      t3.write_cert = write_id
  ));

  log.add(
      common_channel_id,
      transactions::channel_create_transaction(
          channel_id,
          owner_id,
          name,
          read_id,
          write_id));

  log.add(
      owner_id,
      transactions::channel_add_writer_transaction(
          channel_id,
          owner_cert,
          read_cert,
          read_private_key,
          write_cert,
          write_private_key));

  return user_channel(channel_id, read_cert, write_cert);
}

vds::user_channel
vds::user_manager::create_user_channel(transactions::transaction_block &log, database_transaction &t,
                                  const vds::guid &common_channel_id, const vds::guid &owner_id,
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

  certificate_dbo t1;
  t.execute(t1.insert(
      t1.id = read_id,
      t1.parent = cert_control::get_id(owner_cert),
      t1.cert = read_cert.der()
  ));

  orm::certificate_private_key_dbo t2;
  t.execute(t2.insert(
      t2.id = read_id,
      t2.owner_id = cert_control::get_id(owner_cert),
      t2.body = owner_cert.public_key().encrypt(read_private_key.der(std::string()))
  ));

  log.add(
      common_channel_id,
      transactions::user_channel_create_transaction(
          owner_id,
          read_id,
          cert_control::get_id(owner_cert)));

  return user_channel(owner_id, read_cert, owner_cert);
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
          t3.cert_private_key = device_private_key.der(std::string()),
          t3.port = port,
          t3.common_channel_id = common_channel_id));

  certificate_dbo t4;
  t.execute(
      t4.insert(
          t4.id = device_user.id(),
          t4.cert = device_user.user_certificate().der()));

  orm::certificate_private_key_dbo t5;
  t.execute(
      t5.insert(
          t5.id = device_user.id(),
          t5.owner_id = cert_control::get_id(device_user.user_certificate()),
          t5.body = device_user.user_certificate().public_key().encrypt(
              device_private_key.der(std::string()))));

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
  auto st = t.get_reader(t1.select(t1.cert_id, t1.cert_private_key));
  if(!st.execute()){
    throw std::runtime_error("Unable to get current configuration");
  }
  auto user_id = t1.cert_id.get(st);
  device_private_key = asymmetric_private_key::parse_der(t1.cert_private_key.get(st), std::string());

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
  orm::channel_dbo t1;
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

void vds::user_manager::allow_read(
    vds::database_transaction &t,
    const vds::member_user & user,
    const guid & channel_id,
    const certificate & channel_read_cert,
    const asymmetric_private_key & read_private_key) const {

  orm::certificate_private_key_dbo t2;
  t.execute(t2.insert(
      t2.id = cert_control::get_id(channel_read_cert),
      t2.owner_id = cert_control::get_id(user.user_certificate()),
      t2.body = user.user_certificate().public_key().encrypt(read_private_key.der(std::string()))
  ));

  orm::channel_member_dbo t3;
  t.execute(t3.insert(
      t3.channel_id = channel_id,
      t3.user_cert_id = cert_control::get_id(user.user_certificate()),
      t3.member_type = (uint8_t)orm::channel_member_dbo::member_type_t::reader
  ));
}

vds::asymmetric_private_key
vds::user_manager::get_private_key(database_transaction &t, const vds::guid &cert_id, const vds::guid &user_cert_id,
                                   const asymmetric_private_key &user_cert_private_key) {

  orm::certificate_private_key_dbo t1;
  auto st = t.get_reader(t1.select(t1.body).where(
      t1.id == cert_id
      && t1.owner_id == user_cert_id));
  if(!st.execute()){
    throw vds_exceptions::not_found();
  }

  return asymmetric_private_key::parse_der(
      user_cert_private_key.decrypt(t1.body.get(st)),
      std::string());
}

vds::member_user vds::user_manager::create_root_user(transactions::transaction_block &playback, database_transaction &t,
                                                     const guid &common_channel_id, const std::string &root_user_name,
                                                     const std::string &root_password,
                                                     const vds::asymmetric_private_key &root_private_key) {
  auto root_user_id = guid::new_guid();
  auto root_user_cert = _cert_control::create_root(
      root_user_id,
      "User " + root_user_name,
      root_private_key);

  certificate_dbo t2;
  t.execute(t2.insert(
      t2.id = cert_control::get_id(root_user_cert),
      t2.cert = root_user_cert.der()));

  user_dbo t1;
  t.execute(t1.insert(
     t1.id = root_user_id,
     t1.cert_id = cert_control::get_id(root_user_cert),
     t1.private_key = root_private_key.der(root_password),
     t1.login = root_user_name,
     t1.password_hash = hash::signature(
         hash::sha256(),
         root_password.c_str(),
         root_password.length())));


  playback.add(
      common_channel_id,
      transactions::root_user_transaction(
          root_user_id,
          root_user_cert,
          root_user_name,
          root_private_key.der(root_password),
          hash::signature(hash::sha256(), root_password.c_str(), root_password.length())));

  return member_user(new _member_user(root_user_id, root_user_cert));
}

////////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager()
{
}

