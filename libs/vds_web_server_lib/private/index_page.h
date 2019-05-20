#ifndef __VDS_WEB_SERVER_INDEX_PAGE_H_
#define __VDS_WEB_SERVER_INDEX_PAGE_H_

#include "service_provider.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class http_async_serializer;
  class _web_server;
  class user_manager;
  class http_message;

  class index_page
  {
  public:
    static async_task<expected<http_message>> create_channel(
      const service_provider * sp,
      const std::shared_ptr<user_manager> & user_mng,
      const std::shared_ptr<http_async_serializer> & output_stream,

      const http_message & request);

    static async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> create_message(
      const service_provider * sp,
      const std::shared_ptr<user_manager> &user_mng,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_message & request);

    static async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> parse_join_request(
        const service_provider * sp,
        const std::shared_ptr<user_manager>& user_mng,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_message & request);

    static async_task<expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> approve_join_request(
      const service_provider * sp,
      const std::shared_ptr<user_manager>& user_mng,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_message & request);
  };
}

#endif // __VDS_WEB_SERVER_INDEX_PAGE_H_
