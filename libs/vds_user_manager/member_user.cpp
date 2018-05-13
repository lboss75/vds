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
#include "transactions/root_user_transaction.h"
#include "include/transaction_block_builder.h"
#include "private/member_user_p.h"
#include "private/cert_control_p.h"
#include "cert_control.h"

vds::member_user::member_user(_member_user * impl)
  : impl_(impl)
{
}

vds::member_user vds::member_user::create_user(const vds::asymmetric_private_key &owner_user_private_key, const std::string &user_name,
                                               const vds::asymmetric_private_key &private_key)
{
  return this->impl_->create_user(owner_user_private_key, user_name, private_key);
}

const vds::certificate &vds::member_user::user_certificate() const {
  return this->impl_->user_certificate();
}

vds::member_user vds::member_user::import_user(const certificate &user_cert) {
  return member_user(new _member_user(
      user_cert));
}

////////////////////////////////////////////////////////////////////////////////////////////////

vds::_member_user::_member_user(
    const certificate & user_cert)
  : user_cert_(user_cert)
{
}


vds::member_user vds::_member_user::create_user(
  const vds::asymmetric_private_key &owner_user_private_key,
  const std::string &user_name,
  const vds::asymmetric_private_key &private_key)
{
  auto id = guid::new_guid();
  auto cert_id = guid::new_guid();
  auto cert = _cert_control::create_user_cert(
    user_name,
    private_key,
    this->user_cert_,
    owner_user_private_key);

  return member_user(new _member_user(cert));
}

