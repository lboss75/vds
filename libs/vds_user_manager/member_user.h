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
  
  class member_user
  {
  public:
    member_user(_member_user * impl);

    const guid & id() const;
    const certificate & user_certificate() const;

    static member_user by_login(
        class database_transaction & t,
        const std::string & login);

    member_user create_device_user(
        class transaction_block & log,
        const asymmetric_private_key &owner_user_private_key,
        const asymmetric_private_key &private_key,
        const std::string &device_name);

    member_user create_user(
        const asymmetric_private_key &owner_user_private_key,
        const std::string &user_name,
        const std::string &user_password,
        const asymmetric_private_key &private_key);

  private:
    std::shared_ptr<_member_user> impl_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_H_
