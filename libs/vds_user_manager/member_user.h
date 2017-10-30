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

    member_user create_user(
      const asymmetric_private_key & owner_user_private_key,
      const std::string & user_name,
      const std::string & user_password,
      const asymmetric_private_key & private_key);

    user_channel create_channel(
      const std::shared_ptr<iuser_manager_storage> & storage,
      const vds::asymmetric_private_key & owner_user_private_key,
      const std::string & channel_name)const;

  private:
    std::shared_ptr<_member_user> impl_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_H_
