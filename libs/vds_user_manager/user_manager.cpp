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
#include "../vds_db_model/certificate_dbo.h"
#include "../vds_db_model/user_dbo.h"
#include "transactions/root_user_transaction.h"

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

vds::user_channel vds::user_manager::create_channel(
    const vds::member_user &owner,
    const vds::asymmetric_private_key &owner_user_private_key,
    const std::string &name) {
  return this->impl_->create_channel(owner, owner_user_private_key, name);
}

void vds::user_manager::apply_transaction_record(
    const vds::service_provider &sp,
    database_transaction & t,
    uint8_t message_id,
    binary_deserializer & s) {

  switch(message_id){
    case root_user_transaction::message_id:
    {
      root_user_transaction record(s);

      certificate_dbo cert_dbo;

      t.execute(cert_dbo.insert(
          cert_dbo.id = record.id(),
          cert_dbo.cert = record.user_cert().der()));

      user_dbo usr_dbo;
      t.execute(
          usr_dbo.insert(
              usr_dbo.id = record.id(),
              usr_dbo.login = record.login(),
              usr_dbo.password_hash = record.password_hash(),
              usr_dbo.private_key = record.private_key()));

      break;
    }
    default:
      throw std::runtime_error("Invalid record ID");
  }

}

const_data_buffer vds::user_manager::reset(
    const service_provider &sp,
    const std::string &root_user_name,
    const std::string &root_password,
    const asymmetric_private_key &root_private_key) {

  transaction_block log;

  asymmetric_private_key private_key(asymmetric_crypto::rsa2048());
  private_key.generate();

  auto user = this->create_root_user(
      log,
      root_user_name,
      root_password,
      root_private_key);

  auto device_user = user.create_device_user(log, root_private_key);

  log.add(device_user_transaction(
      user.id(),
      device_user.id(),
      device_user.user_certificate());

  return log.sign(user.id(), user.user_certificate(), private_key);
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

vds::user_channel vds::_user_manager::create_channel(
    const vds::member_user &owner,
    const vds::asymmetric_private_key & owner_user_private_key,
    const std::string &name)
{
}