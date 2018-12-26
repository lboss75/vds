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
      const std::shared_ptr<certificate> & user_cert,
      const std::shared_ptr<asymmetric_private_key> &private_key);

    vds::member_user create_user(
      const std::shared_ptr<asymmetric_private_key> &owner_user_private_key,
      const std::string &user_name,
      const std::shared_ptr<asymmetric_private_key> &private_key);

    const std::shared_ptr<certificate> & user_certificate() const {
      return this->user_cert_;
    }

    const std::shared_ptr<asymmetric_private_key> & private_key() const {
      return this->private_key_;
    }

    user_channel create_channel(      
      transactions::transaction_block_builder& log,
      const std::string & channel_type,
      const std::string& name);

    static member_user create_root_user(
      transactions::transaction_block_builder &playback,
      const std::string &root_user_name,
      const std::string &root_password,
      const std::shared_ptr<asymmetric_private_key> &root_private_key);

    user_channel personal_channel() const;

  private:
    std::shared_ptr<certificate> user_cert_;
    std::shared_ptr<asymmetric_private_key> private_key_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_P_H_
