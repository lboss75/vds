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
#include "transaction_block_builder.h"
#include "private/member_user_p.h"
#include "private/cert_control_p.h"
#include "keys_control.h"
#include "dht_object_id.h"
#include "private/user_channel_p.h"
#include "create_user_transaction.h"

vds::member_user::member_user(_member_user * impl)
  : impl_(impl)
{
}

//vds::member_user vds::member_user::create_user(const vds::asymmetric_private_key &owner_user_private_key, const std::string &user_name,
//                                               const vds::asymmetric_private_key &private_key)
//{
//  return this->impl_->create_user(owner_user_private_key, user_name, private_key);
//}

vds::member_user::member_user()
: impl_(nullptr) {
}

vds::member_user::member_user(member_user&& original) noexcept
: impl_(original.impl_){
  original.impl_ = nullptr;
}

vds::member_user::~member_user() {
  delete this->impl_;
}

vds::member_user::member_user(const std::shared_ptr<asymmetric_public_key>& user_cert, const std::shared_ptr<asymmetric_private_key> & private_key)
  : impl_(new _member_user(user_cert, private_key)) {
}

const std::shared_ptr<vds::asymmetric_public_key> &vds::member_user::user_public_key() const {
  return this->impl_->user_certificate();
}

const std::shared_ptr<vds::asymmetric_private_key> & vds::member_user::private_key() const {
  return this->impl_->private_key();
}

vds::expected<vds::user_channel> vds::member_user::create_channel(
  transactions::transaction_block_builder& log,
  const std::string & channel_type, 
  const std::string& name) {
  return this->impl_->create_channel(
    log,
    channel_type,
    name);
}

vds::expected<vds::user_channel> vds::member_user::personal_channel() const {
  return this->impl_->personal_channel();
}

vds::member_user& vds::member_user::operator=(member_user&& original) noexcept {
  delete this->impl_;
  this->impl_ = original.impl_;
  original.impl_ = nullptr;
  return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////

vds::_member_user::_member_user(
  const std::shared_ptr<asymmetric_public_key> & user_cert,
  const std::shared_ptr<asymmetric_private_key> &private_key)
  : user_cert_(user_cert), private_key_(private_key)
{
}

vds::expected<vds::member_user> vds::_member_user::create_user(
  transactions::transaction_block_builder & log,
  const std::string & user_name,
  const std::string & user_email,
  const std::string & user_password,
  const std::shared_ptr<asymmetric_private_key> &private_key)
{
  GET_EXPECTED(c, asymmetric_public_key::create(*private_key));

  auto user_cert = std::make_shared<asymmetric_public_key>(std::move(c));

  GET_EXPECTED(user_private_key_der, private_key->der(user_password));
  GET_EXPECTED(user_id, dht::dht_object_id::user_credentials_to_key(user_email, user_password));
  CHECK_EXPECTED(log.add(
    message_create<transactions::create_user_transaction>(
      user_id,
      user_cert,
      user_private_key_der,
      user_name)));

  return member_user(user_cert, private_key);
}

vds::expected<vds::user_channel> vds::_member_user::create_channel(
  transactions::transaction_block_builder& log,
  const std::string & channel_type,
  const std::string& name) {
  
  GET_EXPECTED(read_key_data, asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  
  auto read_private_key = std::make_shared<asymmetric_private_key>(std::move(read_key_data));

  GET_EXPECTED(write_key_data, asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  auto write_private_key = std::make_shared<asymmetric_private_key>(std::move(write_key_data));

  GET_EXPECTED(read_private_key_der, read_private_key->der(std::string()));
  GET_EXPECTED(channel_id, hash::signature(hash::sha256(), read_private_key_der));
  GET_EXPECTED(read_cert_data, asymmetric_public_key::create(*read_private_key));

  auto read_cert = std::make_shared<asymmetric_public_key>(std::move(read_cert_data));
  GET_EXPECTED(read_id, read_cert->hash(hash::sha256()));
  GET_EXPECTED(write_cert_data, asymmetric_public_key::create(*write_private_key));
  auto write_cert = std::make_shared<asymmetric_public_key>(std::move(write_cert_data));
  GET_EXPECTED(write_id, write_cert->hash(hash::sha256()));

  GET_EXPECTED(pc, (*this).personal_channel());
  CHECK_EXPECTED(pc->add_log(log, 
    message_create<transactions::channel_create_transaction>(
    channel_id,
    channel_type,
    name,
    read_cert,
    read_private_key,
    write_cert,
    write_private_key)));

  return user_channel(
    channel_id,
    channel_type,
    name,
    read_id,
    read_cert,
    read_private_key,
    write_id,
    write_cert,
    write_private_key);
}

vds::expected<vds::user_channel> vds::_member_user::personal_channel() const {
  GET_EXPECTED(fp, this->user_cert_->hash(hash::sha256()));
  GET_EXPECTED(read_id, this->user_cert_->hash(hash::sha256()));
  return user_channel(
    fp,
    user_channel::channel_type_t::personal_channel,
    "!Personal",
    read_id,
    this->user_cert_,
    this->private_key_,
    read_id,
    this->user_cert_,
    this->private_key_);
}

