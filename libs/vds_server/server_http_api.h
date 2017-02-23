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
    server_http_api(const service_provider & sp);


    void start(const std::string & address, int port);

  private:
    service_provider sp_;
    std::unique_ptr<_server_http_api> impl_;
  };
}

#endif // __VDS_SERVER_SERVER_HTTP_API_H_
