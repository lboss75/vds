#ifndef __VDS_USER_MANAGER_USER_MANAGER_STORAGE_H_
#define __VDS_USER_MANAGER_USER_MANAGER_STORAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <const_data_buffer.h>
#include <guid.h>

namespace vds {
  class member_user;
  class user_channel;

  class iuser_manager_storage : public std::enable_shared_from_this<iuser_manager_storage>
  {
  public:
    virtual ~iuser_manager_storage() {}

    virtual member_user new_user(member_user && user) = 0;
    virtual user_channel new_channel(
        user_channel && channel,
        const guid & owner_id_,
        const const_data_buffer & crypted_private_key) = 0;

  };

}

#endif//__VDS_USER_MANAGER_USER_MANAGER_STORAGE_H_
