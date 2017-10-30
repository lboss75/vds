#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>

namespace vds {
  class asymmetric_private_key;
  class member_user;
  class user_channel;
  class iuser_manager_storage;
  class _user_manager;
  
  class user_manager
  {
  public:
    user_manager(const std::shared_ptr<iuser_manager_storage> & storage);
    
    member_user create_root_user(
      const std::string & user_name,
      const std::string & user_password,
      const asymmetric_private_key & private_key);
    
    vds::user_channel create_channel(const vds::member_user &owner,
                                         const vds::asymmetric_private_key &owner_user_private_key,
                                         const std::string &name);
   
  private:
    std::shared_ptr<_user_manager> impl_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
