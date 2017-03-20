/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "certificate_authority.h"
#include "certificate_authority_p.h"

vds::certificate vds::certificate_authority::create_server(
  const certificate & user_certificate,
  const asymmetric_private_key & user_private_key,
  const asymmetric_private_key& server_private_key)
{
  asymmetric_public_key pkey(server_private_key);
  certificate::create_options server_options;
  server_options.country = "RU";
  server_options.organization = "IVySoft";
  server_options.name = "Certificate " + guid::new_guid().str();
  server_options.ca_certificate = &user_certificate;
  server_options.ca_certificate_private_key = &user_private_key;

  return certificate::create_new(pkey, server_private_key, server_options);
}


vds::certificate vds::_certificate_authority::create_root_user(const asymmetric_private_key & private_key)
{
  asymmetric_public_key pkey(private_key);

  certificate::create_options options;
  options.country = "RU";
  options.organization = "IVySoft";
  options.name = "Certificate " + guid::new_guid().str();

  return certificate::create_new(pkey, private_key, options);
}
