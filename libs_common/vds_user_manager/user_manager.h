#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>
#include "transaction_block_builder.h"
#include "encoding.h"
#include "keys_control.h"
#include "user_channel.h"
#include "iserver_api.h"

namespace vds {
  class user_wallet;
  class _user_manager;
  class member_user;
  class vds_client;

  class user_manager : public std::enable_shared_from_this<user_manager>
  {
  public:
    std::shared_ptr<user_channel> get_channel(
      const const_data_buffer &channel_id) const;

    static async_task<expected<void>> reset(
      iserver_api & client,
      const std::string &root_user_name,
      const std::string &root_password,
      const keys_control::private_info_t& private_info);

  private:
    std::map<const_data_buffer, std::shared_ptr<user_channel>> channels_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
