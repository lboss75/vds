/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "member_user.h"
#include "private/member_user_p.h"
#include "user_manager_storage.h"

vds::member_user::member_user(_member_user * impl)
  : impl_(impl)
{
}

vds::member_user vds::member_user::create_user(
  const vds::asymmetric_private_key & owner_user_private_key,
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  return this->impl_->create_user(owner_user_private_key, user_name, user_password, private_key);
}

vds::user_channel vds::member_user::create_channel(
    const std::shared_ptr<iuser_manager_storage> & storage,
    const vds::asymmetric_private_key & owner_user_private_key,
    const std::string & channel_name
)const {
  return this->impl_->create_channel(storage, owner_user_private_key, channel_name);
}


////////////////////////////////////////////////////////////////////////////////////////////////
static vds::crypto_service::certificate_extension_type id_extension_type()
{
  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
    "VDS.ID",
    "VDS Identifier",
    "VDS Identifier");

  return result;
}

static vds::crypto_service::certificate_extension_type parent_id_extension_type()
{
  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
    "VDS.ParentID",
    "VDS Parent Identifier",
    "VDS Parent Identifier");

  return result;
}

static vds::guid certificate_parent_id(const vds::certificate & cert)
{
  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(parent_id_extension_type())).value);
}

vds::_member_user::_member_user(const guid & id, const certificate & user_cert)
  : id_(id), user_cert_(user_cert)
{
}

vds::member_user vds::_member_user::create_root(
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  auto id = guid::new_guid();

  asymmetric_public_key pkey(private_key);

  certificate::create_options options;
  options.country = "RU";
  options.organization = "IVySoft";
  options.name = "User " + user_name;
  options.extensions.push_back(certificate_extension(id_extension_type(), id.str()));

  auto cert = certificate::create_new(pkey, private_key, options);

  return member_user(new _member_user(id, cert));
}

vds::member_user vds::_member_user::create_user(
  const vds::asymmetric_private_key & owner_user_private_key,
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  auto id = guid::new_guid();

  asymmetric_public_key user_pkey(private_key);

  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = "User Certificate " + id.str();
  local_user_options.ca_certificate = &this->user_cert_;
  local_user_options.ca_certificate_private_key = &owner_user_private_key;

  local_user_options.extensions.push_back(certificate_extension(id_extension_type(), id.str()));

  local_user_options.extensions.push_back(
    certificate_extension(parent_id_extension_type(), this->id_.str()));

  auto cert = certificate::create_new(user_pkey, private_key, local_user_options);
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