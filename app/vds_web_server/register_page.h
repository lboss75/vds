#ifndef __VDS_WEB_SERVER_REGISTER_PAGE_H_
#define __VDS_WEB_SERVER_REGISTER_PAGE_H_

#include "service_provider.h"
#include "http_message.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _web_server;

  class register_page
  {
  public:
    static async_task<http_message> post(
      const service_provider& sp,
      const std::shared_ptr<_web_server> & owner,
      const http_message& message);

  private:
    class register_page_task : public std::enable_shared_from_this<register_page_task> {
      
    };
  };
}

#endif // __VDS_WEB_SERVER_REGISTER_PAGE_H_
