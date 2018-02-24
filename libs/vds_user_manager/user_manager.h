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
#include "user_invitation.h"

namespace vds {

  class user_manager
  {
  public:

    user_manager();

    void load(
      const service_provider & sp,
      class database_transaction &t,
      const guid & config_id);

    member_user create_root_user(
      transactions::transaction_block &log,
      class database_transaction &t,
      const guid &common_channel_id,
      const std::string &user_name,
      const std::string &user_password,
      const asymmetric_private_key &private_key);

    user_channel create_channel(
      const service_provider & sp,
      transactions::transaction_block &log, database_transaction &t,
      const vds::guid &channel_id, const std::string &name,
      const vds::guid &owner_id, const certificate &owner_cert,
      const asymmetric_private_key &owner_private_key,
      asymmetric_private_key &read_private_key,
      asymmetric_private_key &write_private_key) const;

    user_invitation reset(const service_provider &sp, class database_transaction &t, const std::string &root_user_name,
      const std::string &root_password, const asymmetric_private_key &root_private_key,
      const std::string &device_name, int port);

    async_task<> init_server(
      const vds::service_provider &sp,
      const user_invitation & request,
      const std::string & user_password,
      const std::string &device_name,
      int port);

    member_user lock_to_device(
      const service_provider &sp,
      class database_transaction &t,
      transactions::transaction_block & playback,
      const std::list<certificate> & certificate_chain,
      const member_user &user,
      const std::string &user_name,
      const std::string &user_password,
      const asymmetric_private_key &user_private_key,
      const std::string &device_name,
      const asymmetric_private_key & device_private_key,
      const guid &common_channel_id,
      int port,
      const certificate & common_channel_read_cert,
      const asymmetric_private_key & common_channel_pkey);

    member_user get_current_device(
      const service_provider &sp,
      asymmetric_private_key & device_private_key) const;

    certificate get_channel_write_cert(const service_provider & sp, const guid &channel_id) const;
    asymmetric_private_key get_channel_write_key(const service_provider & sp, const guid &channel_id) const;
    certificate get_channel_write_cert(const service_provider & sp, const guid & channel_id, const guid & cert_id) const;

    certificate get_channel_read_cert(const service_provider & sp, const guid &channel_id) const;
    asymmetric_private_key get_channel_read_key(const service_provider & sp, const guid &channel_id) const;
    certificate get_certificate(const service_provider & sp, const guid &cert_id) const;

    asymmetric_private_key get_common_channel_read_key(const service_provider & sp, const guid & cert_id) const;

    member_user import_user(const certificate &user_cert);

    user_channel get_channel(const service_provider & sp, const guid &channel_id) const;
    user_channel get_common_channel(const service_provider & sp) const;

    const_data_buffer decrypt_message(
      const service_provider & parent_scope,
      const guid & channel_id,
      int message_id,
      const guid & read_cert_id,
      const guid & write_cert_id,
      const const_data_buffer & message_data,
      const const_data_buffer & signature)  const;

    void apply_channel_message(
      const service_provider & sp,
      const guid & channel_id,
      int message_id,
      const guid & read_cert_id,
      const guid & write_cert_id,
      const const_data_buffer & message,
      const const_data_buffer & signature)  const;

    guid get_common_channel_id(const service_provider & sp) const;

  private:
    std::shared_ptr<class _user_manager> impl_;

  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
