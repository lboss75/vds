#ifndef __VDS_USER_MANAGER_USER_CHANNEL_H_
#define __VDS_USER_MANAGER_USER_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "member_user.h"
#include "const_data_buffer.h"

namespace vds {
  class _user_channel;
  
  class user_channel
  {
  public:
    user_channel();
    ~user_channel();
    
    void add_user(const member_user & user);
    
    const_data_buffer crypt_message(
      const member_user & user,
      const const_data_buffer & message);
    
  private:
    std::shared_ptr<_user_channel> impl_;
  };
}
#endif // __VDS_USER_MANAGER_USER_CHANNEL_H_
