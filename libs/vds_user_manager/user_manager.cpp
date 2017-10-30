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

vds::user_manager::user_manager(const std::shared_ptr<iuser_manager_storage> & storage)
  : impl_(new _user_manager(storage))
{
}

vds::member_user vds::user_manager::create_root_user(
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  return this->impl_->create_root_user(user_name, user_password, private_key);
}

vds::user_channel vds::user_manager::create_channel(
    const vds::member_user &owner,
    const vds::asymmetric_private_key &owner_user_private_key,
    const std::string &name) {
  return this->impl_->create_channel(owner, owner_user_private_key, name);
}

////////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager(const std::shared_ptr<iuser_manager_storage> & storage)
  : storage_(storage)
{
}

vds::member_user vds::_user_manager::create_root_user(
  const std::string & user_name,
  const std::string & user_password,
  const vds::asymmetric_private_key & private_key)
{
  return this->storage_->new_user(_member_user::create_root(user_name, user_password, private_key));
}

vds::user_channel vds::_user_manager::create_channel(
    const vds::member_user &owner,
    const vds::asymmetric_private_key & owner_user_private_key,
    const std::string &name)
{
  return owner.create_channel(this->storage_, owner_user_private_key, name);
}