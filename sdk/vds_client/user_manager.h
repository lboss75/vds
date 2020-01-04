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

namespace vds {
  class user_wallet;
  class _user_manager;
  class member_user;
  class vds_client;

  class user_manager : public std::enable_shared_from_this<user_manager>
  {
  public:
    async_task<expected<void>> reset(
      vds_client& client,
      const std::string &root_user_name,
      const std::string &root_password,
      const keys_control::private_info_t& private_info);

  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
