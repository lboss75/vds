/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "member_user.h"
#include "private/member_user_p.h"
#include "user_manager_storage.h"
#include "database_orm.h"

#ifndef _WIN32
#include <unistd.h>
#include <limits.h>

#endif
#include "transactions/root_user_transaction.h"
#include "transaction_block.h"
#include "private/cert_control_p.h"
#include "transactions/root_user_transaction.h"

vds::member_user::member_user(_member_user * impl)
  : impl_(impl)
{
}

vds::member_user vds::member_user::create_device_user(
    transaction_block & log,
    const vds::asymmetric_private_key &owner_user_private_key,
    const vds::asymmetric_private_key &private_key)
{
  return this->impl_->create_device_user(
      log,
      owner_user_private_key,
      private_key);
}

vds::member_user vds::member_user::create_user(
    const vds::asymmetric_private_key &owner_user_private_key,
    const std::string &user_name,
    const std::string &user_password,
    const vds::asymmetric_private_key &private_key)
{
  return this->impl_->create_user(
      owner_user_private_key,
      user_name, user_password,
      private_key);
}

vds::user_channel vds::member_user::create_channel(
    const std::shared_ptr<iuser_manager_storage> & storage,
    const vds::asymmetric_private_key & owner_user_private_key,
    const std::string & channel_name
)const {
  return this->impl_->create_channel(storage, owner_user_private_key, channel_name);
}


////////////////////////////////////////////////////////////////////////////////////////////////

vds::_member_user::_member_user(const guid & id, const certificate & user_cert)
  : id_(id), user_cert_(user_cert)
{
}

vds::member_user vds::_member_user::create_root(
  transaction_block & log,
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  auto id = guid::new_guid();

  auto cert = _cert_control::create_root(id, "User " + user_name, private_key);

  log.add(root_user_transaction(
      id,
      cert,
      private_key.der(user_password),
      hash::signature(hash::sha256(), user_password.c_str(), user_password.length())));

  return member_user(new _member_user(id, cert));
}

vds::member_user vds::_member_user::create_device_user(
    transaction_block & log,
    const vds::asymmetric_private_key & owner_user_private_key,
    const vds::asymmetric_private_key & private_key) {

#ifndef _WIN32
  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
#else// _WIN32
  CHAR hostname[256];
  DWORD bufCharCount = sizeof(hostname) / sizeof(hostname[0]);
  if(!GetComputerNameA(hostname, &bufCharCount)){
    auto error = GetLastError();
    throw std::system_error(error, std::system_category(), "Get Computer Name");
  }
#endif// _WIN32

  return create_user(
      owner_user_private_key,
      hostname,
      "",
      private_key);
}

vds::member_user vds::_member_user::create_user(
  const vds::asymmetric_private_key & owner_user_private_key,
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  auto id = guid::new_guid();

  auto cert = _cert_control::create(
      id,
      "User Certificate " + id.str(),
      private_key,
      this->id_,
      this->user_cert_,
      owner_user_private_key);

  return member_user(new _member_user(id, cert));
}

vds::user_channel vds::_member_user::create_channel(
    const std::shared_ptr<iuser_manager_storage> & storage,
    const vds::asymmetric_private_key & owner_user_private_key,
    const std::string & channel_name)const
{
  auto id = guid::new_guid();

  asymmetric_private_key private_key(asymmetric_crypto::rsa4096());
  private_key.generate();

  asymmetric_public_key public_key(private_key);

  certificate::create_options cert_options;
  cert_options.country = "RU";
  cert_options.organization = "IVySoft";
  cert_options.name = "Channel Certificate " + id.str();
  cert_options.ca_certificate = &this->user_cert_;
  cert_options.ca_certificate_private_key = &owner_user_private_key;

  cert_options.extensions.push_back(certificate_extension(id_extension_type(), id.str()));

  cert_options.extensions.push_back(
      certificate_extension(parent_id_extension_type(), this->id_.str()));

  auto cert = certificate::create_new(public_key, private_key, cert_options);

  return storage->new_channel(
    user_channel(id, cert),
    this->id_,
    this->user_cert_.public_key().encrypt(private_key.der(std::string())));
}