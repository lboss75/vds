#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>
#include <stdafx.h>
#include "transaction_block.h"
#include "user_channel.h"

namespace vds {

  class user_manager
  {
  public:

    user_manager();
    
    vds::member_user create_root_user(
        transactions::transaction_block &log,
        class database_transaction &t,
        const guid &common_channel_id,
        const std::string &user_name,
        const std::string &user_password,
        const asymmetric_private_key &private_key);

    user_channel create_channel(
        transactions::transaction_block &log,
        class database_transaction & t,
        const guid &common_channel_id,
        const guid &channel_id,
        const std::string &name,
        const guid &owner_id,
        const certificate &owner_cert,
        const asymmetric_private_key &owner_private_key) const;

    user_channel create_user_channel(transactions::transaction_block &log, class database_transaction &t,
                                           const vds::guid &common_channel_id, const vds::guid &owner_id,
                                           const certificate &owner_cert,
                                           const asymmetric_private_key &owner_private_key) const;

    void reset(const service_provider &sp, class database_transaction &t, const std::string &root_user_name,
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
        const member_user &user,
        const std::string &user_name,
        const std::string &user_password,
        const asymmetric_private_key &user_private_key,
        const std::string &device_name,
        const asymmetric_private_key & device_private_key,
        const guid &common_channel_id,
        int port);

    member_user get_current_device(
        const service_provider &sp,
        class database_transaction &t,
        asymmetric_private_key & device_private_key);

    asymmetric_private_key get_channel_write_key(
        const service_provider &sp,
        class database_transaction &t,
        const user_channel &channel_id,
        const guid &user_id);

    certificate get_channel_write_cert(
        const service_provider &sp,
        class database_transaction &t,
        const user_channel & channel,
        const guid &user_id,
        asymmetric_private_key & private_key);

    member_user by_login(class database_transaction &t, const std::string &login);

    member_user import_user(const certificate &user_cert);

    user_channel get_channel(class database_transaction &t, const guid &channel_id) const;
    user_channel get_common_channel(class database_transaction & t) const;

    void allow_read(class database_transaction &t,
                    const member_user & user,
                    const guid & channel_id,
                    const certificate & channel_read_cert,
                    const asymmetric_private_key & read_private_key) const;

  private:
    std::shared_ptr<class _user_manager> impl_;


    asymmetric_private_key
    get_private_key(class database_transaction &t, const vds::guid &cert_id,
                        const vds::guid &user_cert_id,
                        const asymmetric_private_key &user_cert_private_key);
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
