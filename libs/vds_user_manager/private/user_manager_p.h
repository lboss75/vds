#ifndef __VDS_USER_MANAGER_USER_MANAGER_P_H_
#define __VDS_USER_MANAGER_USER_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "user_manager.h"

namespace vds {
  class _user_manager
  {
  public:
    _user_manager();

    /*
     *              *  Admin  * Reader  * Writer
     * Admin cert   *         *
     * Change cert  *
     * Read cert    *
     *
     * Sign = allow to write
     * Read = allow to read
     * Allow write = access to private key
     * Allow read = access to
     */
    vds::user_channel create_channel(transaction_block &log, const guid &common_channel_id, const vds::member_user &owner,
                                         const vds::asymmetric_private_key &owner_user_private_key, const std::string &name,
                                         const asymmetric_private_key &read_private_key,
                                         const asymmetric_private_key &write_private_key);

  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_P_H_
