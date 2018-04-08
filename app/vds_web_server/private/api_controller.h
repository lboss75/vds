#ifndef __VDS_WEB_SERVER_API_CONTROLLER_H_
#define __VDS_WEB_SERVER_API_CONTROLLER_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "http_message.h"
#include "service_provider.h"
#include "web_server_p.h"
#include "user_channel.h"

namespace vds {
  class api_controller {
  public:
    static std::shared_ptr<json_value> get_channels(
        const service_provider& sp,
        user_manager & user_mng,
        const std::shared_ptr<_web_server> & owner,
        const http_message& message);

    static async_task<http_message> get_login_state(
      const service_provider& sp,
      user_manager& user_mng,
      const std::shared_ptr<_web_server>& owner,
      const http_message& message);

    static async_task <http_message> create_channel(
      const service_provider &sp,
      const std::shared_ptr<user_manager> &user_mng,
      user_channel::channel_type_t channel_type,
      const std::string & name);

    static async_task<std::shared_ptr<json_value>> channel_feed(
      const service_provider& sp,
      const std::shared_ptr<user_manager> & user_mng,
      const std::shared_ptr<_web_server>& owner,
      const const_data_buffer & channel_id);

  private:
    static std::shared_ptr<json_object> channel_serialize(const vds::user_channel & channel);

  };
}

#endif //__VDS_WEB_SERVER_API_CONTROLLER_H_
