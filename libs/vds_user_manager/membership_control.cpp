/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/membership_control_p.h"
#include "private/cert_control_p.h"

vds::certificate vds::_membership_control::add_member(
    const certificate & current_cert,
    const certificate & member_cert,

    const guid & admin_id,
    const certificate & admin_cert,
    const asymmetric_private_key & admin_private_key) {

  auto id = guid::new_guid();
  auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());

  auto cert = _cert_control::create(
      id,
      "Member Certificate " + id.str(),
      private_key,
      admin_id,
      admin_cert,
      admin_private_key);

  current_cert.public_key().encrypt(private_key.der(std::string()));

  return certificate();
}
