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
      const guid & id,
      const certificate & user_cert);

    static member_user create_root(
      class transaction_block & log,
      const std::string & user_name,
      const std::string & user_password,
      const vds::asymmetric_private_key & private_key);

    member_user create_user(
        const vds::asymmetric_private_key & owner_user_private_key,
        const std::string & user_name,
        const std::string & user_password,
        const vds::asymmetric_private_key & private_key);

    user_channel create_channel(
        const std::shared_ptr<iuser_manager_storage> & storage,
        const vds::asymmetric_private_key & owner_user_private_key,
        const std::string & channel_name) const;

    member_user create_device_user(
        class transaction_block & log,
        const vds::asymmetric_private_key & owner_user_private_key,
        const vds::asymmetric_private_key & private_key,
        const std::string &device_name);

    const guid & id() const { return this->id_; }
    const certificate & user_certificate() const { return this->user_cert_; }

  private:
    guid id_;
    certificate user_cert_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_P_H_
