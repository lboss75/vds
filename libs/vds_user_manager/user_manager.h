#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>

namespace vds {

  class user_manager
  {
  public:
    user_manager();
    
    class member_user create_root_user(
      class database_transaction & t,
      const std::string & user_name,
      const std::string & user_password,
      const class asymmetric_private_key & private_key);
    
    class user_channel create_channel(
        const member_user &owner,
        const class asymmetric_private_key &owner_user_private_key,
        const std::string &name);
   
  private:
    std::shared_ptr<class _user_manager> impl_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
