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

    async_task<expected<std::shared_ptr<json_value>>> broadcast(transactions::transaction_block_builder & data);

    struct channel_message {
      int64_t id;
      const_data_buffer block_id;
      const_data_buffer channel_id;
      const_data_buffer read_id;
      const_data_buffer write_id;
      const_data_buffer crypted_key;
      const_data_buffer crypted_data;
      const_data_buffer signature;
    };
    
    async_task<expected<void>> walk_messages(
      const const_data_buffer & channel_id,
      lambda_holder_t<async_task<expected<bool>>, channel_message> callback);

    async_task<expected<const_data_buffer>> get_user_profile(const std::string & user_email);
  private:
    std::shared_ptr<websocket_client> ws_;

    int last_id_;
    std::map<int, async_result<expected<std::shared_ptr<json_value>>>> callbacks_;

    async_task<expected<std::shared_ptr<json_value>>> invoke(const std::string& method, std::shared_ptr<json_array> args);

  };
}

#endif // __VDS_USER_MANAGER_USER_PROFILE_H_
