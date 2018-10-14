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
  class http_request;

  class index_page
  {
  public:
    static vds::async_task<http_message> create_channel(
      const vds::service_provider * sp,
      const std::shared_ptr<user_manager> & user_mng,
      const http_request & request);

    static vds::async_task<http_message> create_message(
      const vds::service_provider * sp,
      const std::shared_ptr<user_manager> &user_mng,
      const http_request & request);

    static vds::async_task<std::shared_ptr<vds::json_value>> parse_join_request(
        const vds::service_provider * sp,
        const std::shared_ptr<user_manager>& user_mng,
        const http_request & request);

    static vds::async_task<vds::http_message> approve_join_request(
      const vds::service_provider * sp,
      const std::shared_ptr<user_manager>& user_mng,
      const http_request & request);
  };
}

#endif // __VDS_WEB_SERVER_INDEX_PAGE_H_
