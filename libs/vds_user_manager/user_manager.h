#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "member_user.h"
#include "user_channel.h"

namespace vds {
  class _user_manager;
  
  class user_manager
  {
  public:
    
    member_user create_root_user(
      const std::string & user_name,
      const std::string & user_password); 
    
    member_user create_user(
      const member_user & owner_user,
      const std::string & user_name,
      const std::string & user_password); 
    
    user_channel create_channel(
      const member_user & owner_user,
      const std::string & channel_name); 

  private:
    std::shared_ptr<_user_manager> impl_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
