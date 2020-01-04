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
      const std::shared_ptr<asymmetric_public_key> & user_public_key,
      const std::shared_ptr<asymmetric_private_key> &private_key);

    static async_task<expected<member_user>> create_user(
      vds_client & client,
      transactions::transaction_block_builder & log,
      const std::string & user_name,
      const std::string & user_email,
      const std::string & user_password,
      const std::shared_ptr<asymmetric_private_key> &private_key);

    const std::shared_ptr<asymmetric_public_key> & user_certificate() const {
      return this->user_cert_;
    }

    const std::shared_ptr<asymmetric_private_key> & private_key() const {
      return this->private_key_;
    }

    expected<user_channel> create_channel(
      transactions::transaction_block_builder& log,
      const std::string & channel_type,
      const std::string& name);

    expected<user_channel> personal_channel() const;

  private:
    std::shared_ptr<asymmetric_public_key> user_cert_;
    std::shared_ptr<asymmetric_private_key> private_key_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_P_H_
