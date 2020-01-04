#ifndef __VDS_vds_client_vds_client_H_
#define __VDS_USER_MANAGER_USER_PROFILE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "websocket_client.h"
#include "json_object.h"

namespace vds {
  namespace transactions {
    class transaction_block_builder;
  }
  
  class vds_client
  {
  public:
    vds_client();

    async_task<expected<std::vector<const_data_buffer>>> upload_data(const const_data_buffer& data);

    async_task<expected<void>> broadcast(transactions::transaction_block_builder & data);

  private:
    std::shared_ptr<websocket_client> ws_;

    int last_id_;
    std::map<int, async_result<expected<std::shared_ptr<json_value>>>> callbacks_;

    async_task<expected<void>> invoke(const std::string& method, std::shared_ptr<json_array> args);

  };
}

#endif // __VDS_USER_MANAGER_USER_PROFILE_H_
