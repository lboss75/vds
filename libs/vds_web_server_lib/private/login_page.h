#ifndef __VDS_WEB_SERVER_LOGIN_PAGE_H_
#define __VDS_WEB_SERVER_LOGIN_PAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class service_provider;
  class _web_server;
  class http_message;
  class http_async_serializer;

  class login_page
  {
  public:

    static vds::async_task<vds::expected<vds::stream_output_async<uint8_t>>> register_request_post(
      const service_provider * sp,
		  const std::shared_ptr<http_async_serializer> & output_stream,
		  const std::shared_ptr<_web_server>& owner,
      const http_message& message);
  };
}

#endif // __VDS_WEB_SERVER_LOGIN_PAGE_H_
