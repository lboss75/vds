#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>
#include <transaction_block.h>
#include <stdafx.h>
#include "user_channel.h"

namespace vds {

  class user_manager
  {
  public:

    user_manager();
    
    class member_user create_root_user(
      transaction_block & log,
      const std::string & user_name,
      const std::string & user_password,
      const class asymmetric_private_key & private_key);
    
    vds::user_channel create_channel(transaction_block &log, const vds::member_user &owner,
                                         const vds::asymmetric_private_key &owner_user_private_key,
                                         const std::string &name,
                                         const asymmetric_private_key &read_private_key,
                                         const asymmetric_private_key &write_private_key);

    vds::const_data_buffer reset(const service_provider &sp, class database_transaction &t, const std::string &root_user_name,
                                     const std::string &root_password, const asymmetric_private_key &root_private_key,
                                 const std::string &device_name, int port);

    void apply_transaction_record(
        const service_provider &sp,
        class database_transaction & t,
        uint8_t message_id,
        binary_deserializer & s);

    member_user lock_to_device(
        const service_provider &sp,
        class database_transaction &t,
        class transaction_block &log,
        const member_user &user,
        const std::string &user_name,
        const std::string &user_password,
        const asymmetric_private_key &user_private_key,
        const std::string &device_name,
        const asymmetric_private_key & device_private_key,
        int port);

    member_user by_login(class database_transaction &t, const std::string &login);

    member_user import_user(const certificate &user_cert);

  private:
    std::shared_ptr<class _user_manager> impl_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
