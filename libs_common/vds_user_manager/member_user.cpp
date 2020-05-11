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
#include "transaction_block_builder.h"
#include "private/member_user_p.h"
#include "private/cert_control_p.h"
#include "keys_control.h"
#include "private/user_channel_p.h"
#include "create_user_transaction.h"
#include "user_profile.h"
#include "iserver_api.h"
#include "store_block_transaction.h"

vds::member_user::member_user(_member_user * impl)
  : impl_(impl)
{
}

vds::async_task<vds::expected<vds::member_user>> vds::member_user::create_user(
  iserver_api & client,
  transactions::transaction_block_builder& log,
  const std::string& user_name,
  const std::string& user_email,
  const std::string& user_password,
  const std::shared_ptr<vds::asymmetric_private_key> & private_key)
{
  return this->impl_->create_user(client, log, user_name, user_email, user_password, private_key);
}

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

vds::async_task<vds::expected<vds::member_user>> vds::_member_user::create_user(
  iserver_api & client,
  transactions::transaction_block_builder & log,
  const std::string & user_name,
  const std::string & user_email,
  const std::string & user_password,
  const std::shared_ptr<asymmetric_private_key> &private_key)
{
  GET_EXPECTED_ASYNC(user_private_key_der, private_key->der(user_password));
  GET_EXPECTED_ASYNC(password_hash,
    hash::signature(
      hash::sha256(),
      const_data_buffer(user_password.c_str(), user_password.length())));
  GET_EXPECTED_ASYNC(user_profile_data, message_create<user_profile>(
    password_hash,
    user_private_key_der));
  GET_EXPECTED_ASYNC(profile_block, message_serialize<user_profile>(user_profile_data));
  GET_EXPECTED_ASYNC(profile_info, co_await client.upload_data(profile_block));

  GET_EXPECTED_ASYNC(user_public_key, asymmetric_public_key::create(*private_key));

  auto user_pkey = std::make_shared<asymmetric_public_key>(std::move(user_public_key));
  GET_EXPECTED_ASYNC(user_id, user_pkey->fingerprint());

  CHECK_EXPECTED_ASYNC(log.add(
    transactions::store_block_transaction::create(
      user_id,
      profile_info.data_hash,
      profile_block.size(),
      profile_info.replica_size,
      profile_info.replicas,
      *private_key)));

  CHECK_EXPECTED_ASYNC(log.add(
    message_create<transactions::create_user_transaction>(
      user_email,
      user_pkey,
      profile_info.data_hash,
      user_name)));

  co_return member_user(user_pkey, private_key);
}

vds::expected<vds::user_channel> vds::_member_user::create_channel(
  transactions::transaction_block_builder& log,
  const std::string & channel_type,
  const std::string& name) {
  
  GET_EXPECTED(read_key_data, asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  auto read_private_key = std::make_shared<asymmetric_private_key>(std::move(read_key_data));

  GET_EXPECTED(write_key_data, asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  auto write_private_key = std::make_shared<asymmetric_private_key>(std::move(write_key_data));

  GET_EXPECTED(admin_key_data, asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  auto admin_private_key = std::make_shared<asymmetric_private_key>(std::move(admin_key_data));


  GET_EXPECTED(read_cert_data, asymmetric_public_key::create(*read_private_key));
  auto read_cert = std::make_shared<asymmetric_public_key>(std::move(read_cert_data));
  GET_EXPECTED(read_id, read_cert->fingerprint());

  GET_EXPECTED(write_cert_data, asymmetric_public_key::create(*write_private_key));
  auto write_cert = std::make_shared<asymmetric_public_key>(std::move(write_cert_data));
  GET_EXPECTED(write_id, write_cert->fingerprint());

  GET_EXPECTED(admin_cert_data, asymmetric_public_key::create(*admin_private_key));
  auto admin_cert = std::make_shared<asymmetric_public_key>(std::move(admin_cert_data));
  GET_EXPECTED(admin_id, admin_cert->fingerprint());

  GET_EXPECTED(pc, (*this).personal_channel());
  CHECK_EXPECTED(pc->add_log(log, 
    message_create<transactions::channel_create_transaction>(
    channel_type,
    name,
    read_cert,
    read_private_key,
    write_cert,
    write_private_key,
    admin_cert,
    admin_private_key)));

  return user_channel(
    channel_type,
    name,
    read_id,
    read_cert,
    read_private_key,
    write_id,
    write_cert,
    write_private_key,
    admin_id,
    admin_cert,
    admin_private_key);
}

vds::expected<vds::user_channel> vds::_member_user::personal_channel() const {
  GET_EXPECTED(fp, this->user_cert_->fingerprint());
  return user_channel(
    user_channel::channel_type_t::personal_channel,
    "!Personal",
    fp,
    this->user_cert_,
    this->private_key_,
    fp,
    this->user_cert_,
    this->private_key_,
    fp,
    this->user_cert_,
    this->private_key_);
}

