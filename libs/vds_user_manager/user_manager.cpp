/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "user_manager.h"
#include "member_user.h"
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
  return this->storage_->new_user(member_user(_member_user::create_root(user_name, user_password, private_key)));
}

////////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager(const std::shared_ptr<iuser_manager_storage> & storage)
  : storage_(storage)
{
}
