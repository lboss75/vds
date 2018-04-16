#ifndef __VDS_USER_MANAGER_MEMBER_USER_P_H_
#define __VDS_USER_MANAGER_MEMBER_USER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>

namespace vds {

  class _member_user
  {
  public:
    _member_user(
      const certificate & user_cert);

    vds::member_user create_user(
      const vds::asymmetric_private_key &owner_user_private_key,
      const std::string &user_name,
      const vds::asymmetric_private_key &private_key);

    const certificate & user_certificate() const { return this->user_cert_; }

  private:
    certificate user_cert_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_P_H_
