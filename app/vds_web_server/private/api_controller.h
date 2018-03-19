#ifndef __VDS_WEB_SERVER_API_CONTROLLER_H_
#define __VDS_WEB_SERVER_API_CONTROLLER_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include <http_message.h>
#include <service_provider.h>
#include "web_server_p.h"

namespace vds {
  class api_controller {
  public:
    static async_task<http_message> get_channels(
        const service_provider& sp,
        user_manager & user_mng,
        const std::shared_ptr<_web_server> & owner,
        const http_message& message);

    static async_task<http_message> get_login_state(
      const service_provider& sp,
      user_manager& user_mng,
      const std::shared_ptr<_web_server>& owner,
      const http_message& message);

  };
}

#endif //__VDS_WEB_SERVER_API_CONTROLLER_H_
