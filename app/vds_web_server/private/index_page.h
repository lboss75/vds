#ifndef __VDS_WEB_SERVER_INDEX_PAGE_H_
#define __VDS_WEB_SERVER_INDEX_PAGE_H_

#include "service_provider.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _web_server;
  class user_manager;
  class http_message;

  class index_page
  {
  public:
    static vds::async_task<http_message> create_channel(
      const vds::service_provider &sp,
      const std::shared_ptr<user_manager> &user_mng,
      const std::shared_ptr<_web_server> &web_server,
      const http_message &message);

  };
}

#endif // __VDS_WEB_SERVER_INDEX_PAGE_H_
