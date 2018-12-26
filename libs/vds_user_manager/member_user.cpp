/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"

#ifndef _WIN32
#include <unistd.h>
#include <limits.h>
#endif

#include "member_user.h"
#include "database_orm.h"
#include "user_manager_transactions.h"
#include "transaction_block_builder.h"
#include "private/member_user_p.h"
#include "private/cert_control_p.h"
#include "cert_control.h"
#include "dht_object_id.h"
#include "private/user_channel_p.h"

vds::member_user::member_user(_member_user * impl)
  : impl_(impl)
{
}

//vds::member_user vds::member_user::create_user(const vds::asymmetric_private_key &owner_user_private_key, const std::string &user_name,
//                                               const vds::asymmetric_private_key &private_key)
//{
//  return this->impl_->create_user(owner_user_private_key, user_name, private_key);
//}

vds::member_user::member_user(const std::shared_ptr<certificate>& user_cert, const std::shared_ptr<asymmetric_private_key> & private_key)
  : impl_(new _member_user(user_cert, private_key)) {
}

const std::shared_ptr<vds::certificate> &vds::member_user::user_certificate() const {
  return this->impl_->user_certificate();
}

const std::shared_ptr<vds::asymmetric_private_key> & vds::member_user::private_key() const {
  return this->impl_->private_key();
}

vds::user_channel vds::member_user::create_channel(
  transactions::transaction_block_builder& log,
  const std::string & channel_type, 
  const std::string& name) {
  return this->impl_->create_channel(
    log,
    channel_type,
    name);
}

vds::user_channel vds::member_user::personal_channel() const {
  return this->impl_->personal_channel();
}

////////////////////////////////////////////////////////////////////////////////////////////////

vds::_member_user::_member_user(
  const std::shared_ptr<certificate> & user_cert,
  const std::shared_ptr<asymmetric_private_key> &private_key)
  : user_cert_(user_cert), private_key_(private_key)
{
}


vds::member_user vds::_member_user::create_user(
  const std::shared_ptr<asymmetric_private_key> &owner_user_private_key,
  const std::string &user_name,
  const std::shared_ptr<asymmetric_private_key> &private_key)
{
  auto cert = std::make_shared<certificate>(_cert_control::create_user_cert(
    user_name,
    *private_key,
    *this->user_cert_,
    *owner_user_private_key));

  return member_user(cert, private_key);
}

vds::user_channel vds::_member_user::create_channel(
  transactions::transaction_block_builder& log,
  const std::string & channel_type,
  const std::string& name) {

  auto read_private_key = std::make_shared<asymmetric_private_key>(asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  auto write_private_key = std::make_shared<asymmetric_private_key>(asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));

  auto channel_id = hash::signature(hash::sha256(), read_private_key->der(std::string()));

  auto read_cert = std::make_shared<certificate>(_cert_control::create_cert(
    base64::from_bytes(channel_id) + "(Read)",
    *read_private_key,
    *this->user_cert_,
    *this->private_key_));

  auto write_cert = std::make_shared<certificate>(vds::_cert_control::create_cert(
    base64::from_bytes(channel_id) + "(Write)",
    *write_private_key,
    *this->user_cert_,
    *this->private_key_));

  (*this).personal_channel()->add_log(
    log,
    message_create<transactions::channel_create_transaction>(
      channel_id,
      channel_type,
      name,
      read_cert,
      read_private_key,
      write_cert,
      write_private_key));

  return user_channel(
    channel_id,
    channel_type,
    name,
    read_cert,
    read_private_key,
    write_cert,
    write_private_key);

}

vds::member_user vds::_member_user::create_root_user(
  transactions::transaction_block_builder& playback,
  const std::string& root_user_name,
  const std::string& root_password,
  const std::shared_ptr<vds::asymmetric_private_key> & root_private_key) {

  const auto root_user_cert = cert_control::get_root_certificate();

  playback.add(
    message_create<transactions::root_user_transaction>(
      dht::dht_object_id::user_credentials_to_key(root_user_name, root_password),
      root_user_cert,
      root_user_name,
      root_private_key->der(root_password)));

  return member_user(root_user_cert, root_private_key);
}

vds::user_channel vds::_member_user::personal_channel() const {
  return user_channel(
    this->user_cert_->fingerprint(hash::sha256()),
    user_channel::channel_type_t::personal_channel,
    "!Personal",
    this->user_cert_,
    this->private_key_,
    this->user_cert_,
    this->private_key_);
}

