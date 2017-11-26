#ifndef __VDS_USER_MANAGER_USER_CHANNEL_H_
#define __VDS_USER_MANAGER_USER_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <guid.h>
#include "const_data_buffer.h"
#include "stdafx.h"

namespace vds {
  class member_user;

  class user_channel
  {
  public:
    user_channel();

    user_channel(
        const guid & id,
        const vds::certificate & read_cert,
        const vds::certificate & write_cert);


    void add_user(const member_user & user);
    
    const_data_buffer crypt_message(
      const member_user & user,
      const const_data_buffer & message);

    const vds::guid &id() const;
    const vds::certificate & read_cert() const;
    const vds::certificate & write_cert() const;

  private:
    std::shared_ptr<class _user_channel> impl_;
  };
}
#endif // __VDS_USER_MANAGER_USER_CHANNEL_H_
