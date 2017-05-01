#ifndef __VDS_SERVER_SERVER_HTTP_API_H_
#define __VDS_SERVER_SERVER_HTTP_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _server_http_api;

  class server_http_api
  {
  public:
    server_http_api();


    async_task<> start(
      const service_provider & sp,
      const std::string & address,
      int port,
      certificate & certificate,
      asymmetric_private_key & private_key);

  private:
    std::unique_ptr<_server_http_api> impl_;
  };
}

#endif // __VDS_SERVER_SERVER_HTTP_API_H_
