#ifndef __VDS_WEB_SERVER_LOGIN_PAGE_H_
#define __VDS_WEB_SERVER_LOGIN_PAGE_H_

#include "service_provider.h"
#include "http_message.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _web_server;

  class login_page
  {
  public:

    static vds::async_task<http_message> register_request_post(
      const service_provider& sp,
      const std::shared_ptr<_web_server>& owner,
      const http_message& message);
  };
}

#endif // __VDS_WEB_SERVER_LOGIN_PAGE_H_
