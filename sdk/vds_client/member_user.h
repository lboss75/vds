#ifndef __VDS_USER_MANAGER_MEMBER_USER_H_
#define __VDS_USER_MANAGER_MEMBER_USER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "user_channel.h"

namespace vds {
  class asymmetric_private_key;
  class iuser_manager_storage;
  class _member_user;
  class vds_client;
  
  class member_user
  {
  public:
    member_user();
    member_user(_member_user * impl);
    member_user(const member_user &) = delete;
    member_user(member_user && original) noexcept;
    ~member_user();

    member_user(
      const std::shared_ptr<asymmetric_public_key> &user_public_key,
      const std::shared_ptr<asymmetric_private_key> & private_key);

    const std::shared_ptr<asymmetric_public_key> & user_public_key() const;
    const std::shared_ptr<asymmetric_private_key> & private_key() const;

    async_task<expected<member_user>> create_user(
      vds_client& client,
      transactions::transaction_block_builder& log,
      const std::string & user_name,
      const std::string & user_email,
      const std::string& user_password,
      const std::shared_ptr<vds::asymmetric_private_key> &private_key);

    expected<user_channel> create_channel(
      transactions::transaction_block_builder &log,
      const std::string & channel_type,
      const std::string &name);

    expected<user_channel> personal_channel() const;

    _member_user * operator -> () const {
      return this->impl_;
    }

    member_user & operator = (const member_user &) = delete;
    member_user & operator = (member_user &&) noexcept;

  private:
    _member_user * impl_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_H_
