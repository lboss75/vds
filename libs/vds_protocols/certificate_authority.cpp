/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "certificate_authority.h"
#include "private/certificate_authority_p.h"

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


vds::certificate vds::certificate_authority::create_server(
  const guid & server_id,
  const certificate & user_certificate,
  const asymmetric_private_key & user_private_key,
  const asymmetric_private_key& server_private_key)
{
  asymmetric_public_key pkey(server_private_key);
  certificate::create_options server_options;
  server_options.country = "RU";
  server_options.organization = "IVySoft";
  server_options.name = "Certificate " + server_id.str();
  server_options.ca_certificate = &user_certificate;
  server_options.ca_certificate_private_key = &user_private_key;

  server_options.extensions.push_back(certificate_extension(id_extension_type(), server_id.str()));

  return certificate::create_new(pkey, server_private_key, server_options);
}

vds::guid vds::certificate_authority::certificate_id(const certificate & cert)
{
  return guid::parse(cert.get_extension(cert.extension_by_NID(id_extension_type())).value);
}

vds::guid vds::certificate_authority::certificate_parent_id(const certificate & cert)
{
  return guid::parse(cert.get_extension(cert.extension_by_NID(parent_id_extension_type())).value);
}

vds::certificate vds::certificate_authority::create_local_user(
  const guid & user_id,
  const certificate & owner_certificate,
  const asymmetric_private_key & owner_private_key,
  const asymmetric_private_key & user_private_key)
{
  asymmetric_public_key local_user_pkey(user_private_key);

  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = "Local User Certificate " + user_id.str();
  local_user_options.ca_certificate = &owner_certificate;
  local_user_options.ca_certificate_private_key = &owner_private_key;

  local_user_options.extensions.push_back(certificate_extension(id_extension_type(), user_id.str()));

  local_user_options.extensions.push_back(
    certificate_extension(parent_id_extension_type(),
    certificate_parent_id(owner_certificate).str()));

  return certificate::create_new(local_user_pkey, user_private_key, local_user_options);
}

vds::certificate vds::_certificate_authority::create_root_user(
  const guid & id,
  const asymmetric_private_key & private_key)
{
  asymmetric_public_key pkey(private_key);

  certificate::create_options options;
  options.country = "RU";
  options.organization = "IVySoft";
  options.name = "Certificate " + id.str();
  options.extensions.push_back(certificate_extension(id_extension_type(), id.str()));

  return certificate::create_new(pkey, private_key, options);
}
