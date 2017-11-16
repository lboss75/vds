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

vds::user_manager::user_manager()
  : impl_(new _user_manager())
{
}

vds::member_user vds::user_manager::create_root_user(
  database_transaction & t,
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  return this->impl_->create_root_user(t, user_name, user_password, private_key);
}

vds::user_channel vds::user_manager::create_channel(
    const vds::member_user &owner,
    const vds::asymmetric_private_key &owner_user_private_key,
    const std::string &name) {
  return this->impl_->create_channel(owner, owner_user_private_key, name);
}

////////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager()
{
}

vds::member_user vds::_user_manager::create_root_user(
  database_transaction & t,
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  auto user = _member_user::create_root(user_name, user_password, private_key);

  certificate_dbo cert_dbo;

  t.execute(cert_dbo.insert(
      cert_dbo.id = user.id(),
      cert_dbo.cert = user.user_certificate().der()));

  user_dbo usr_dbo;
  t.execute(
      usr_dbo.insert(
        usr_dbo.id = user.id(),
        usr_dbo.login = user_name,
        usr_dbo.password_hash = hash::signature(
            hash::sha256(),
            user_password.c_str(),
            user_password.length()),
        usr_dbo.private_key = private_key.der(user_password)));

  return user;
}

vds::user_channel vds::_user_manager::create_channel(
    const vds::member_user &owner,
    const vds::asymmetric_private_key & owner_user_private_key,
    const std::string &name)
{
}