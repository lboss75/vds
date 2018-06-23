#ifndef __VDS_USER_MANAGER_MEMBER_USER_P_H_
#define __VDS_USER_MANAGER_MEMBER_USER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>

namespace vds {
  class user_channel;

  class _member_user
  {
  public:
    _member_user(
      const certificate & user_cert,
      const asymmetric_private_key &private_key);

    vds::member_user create_user(
      const asymmetric_private_key &owner_user_private_key,
      const std::string &user_name,
      const asymmetric_private_key &private_key);

    const certificate & user_certificate() const {
      return this->user_cert_;
    }

    const asymmetric_private_key & private_key() const {
      return this->private_key_;
    }

    user_channel create_channel(
      const service_provider& sp,
      transactions::transaction_block_builder& log,
      const std::string& name);

    static member_user create_root_user(
      const service_provider & sp,
      transactions::transaction_block_builder &playback,
      database_transaction & t,
      const std::string &root_user_name,
      const std::string &root_password,
      const vds::asymmetric_private_key &root_private_key);

    user_channel personal_channel() const;

  private:
    certificate user_cert_;
    asymmetric_private_key private_key_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_P_H_
