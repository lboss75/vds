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
#include "user_manager_storage.h"
#include "database_orm.h"
#include "user_dbo.h"
#include "certificate_dbo.h"
#include "transactions/root_user_transaction.h"
#include "transaction_block.h"
#include "private/member_user_p.h"
#include "private/cert_control_p.h"
#include "cert_control.h"

vds::member_user::member_user(_member_user * impl)
  : impl_(impl)
{
}

vds::member_user vds::member_user::create_device_user(
    const vds::asymmetric_private_key &owner_user_private_key,
    const vds::asymmetric_private_key &private_key,
    const std::string &device_name) const
{
  return this->impl_->create_device_user(owner_user_private_key, private_key, device_name);
}

vds::member_user vds::member_user::create_user(const vds::asymmetric_private_key &owner_user_private_key, const std::string &user_name,
                                               const vds::asymmetric_private_key &private_key)
{
  return this->impl_->create_user(owner_user_private_key, user_name, private_key);
}

const vds::guid &vds::member_user::id() const {
  return this->impl_->id();
}

const vds::certificate &vds::member_user::user_certificate() const {
  return this->impl_->user_certificate();
}

vds::member_user vds::member_user::by_login(vds::database_transaction &t, const std::string &login) {

  dbo::user_dbo t1;
  dbo::certificate t2;
  auto st = t.get_reader(t1
                             .select(t1.id, t2.cert)
                             .inner_join(t2, t2.id == t1.cert_id)
                             .where(t1.login == login));
  if(!st.execute()){
    throw std::runtime_error("User with login '" + login + "' was not found");
  }

  return member_user(new _member_user(
      t1.id.get(st),
      certificate::parse_der(t2.cert.get(st))));
}

vds::member_user vds::member_user::import_user(const certificate &user_cert) {
  return member_user(new _member_user(
      cert_control::get_id(user_cert),
      user_cert));
}

////////////////////////////////////////////////////////////////////////////////////////////////

vds::_member_user::_member_user(
    const guid & id,
    const certificate & user_cert)
  : id_(id),
    user_cert_(user_cert)
{
}

vds::member_user vds::_member_user::create_device_user(const vds::asymmetric_private_key &owner_user_private_key,
                                                       const vds::asymmetric_private_key &private_key,
                                                       const std::string &device_name) {

	auto name = device_name;
#ifndef _WIN32
  if(name.empty()) {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    name = hostname;
  }
#else// _WIN32
  CHAR hostname[256];
  DWORD bufCharCount = sizeof(hostname) / sizeof(hostname[0]);
  if(!GetComputerNameA(hostname, &bufCharCount)){
    auto error = GetLastError();
    throw std::system_error(error, std::system_category(), "Get Computer Name");
  }
  name = hostname;
#endif// _WIN32

  return create_user(owner_user_private_key, name, private_key);
}

vds::member_user vds::_member_user::create_user(const vds::asymmetric_private_key &owner_user_private_key, const std::string &user_name,
                                                const vds::asymmetric_private_key &private_key)
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

