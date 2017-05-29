#ifndef __VDS_SERVER_SERVER_JSON_CLIENT_API_H_
#define __VDS_SERVER_SERVER_JSON_CLIENT_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _server_json_client_api;

  class server_json_client_api
  {
  public:
    server_json_client_api();
    ~server_json_client_api();

    std::shared_ptr<json_value> operator()(const service_provider & scope, const std::shared_ptr<json_value> & request) const;

  private:
    _server_json_client_api * const impl_;
  };
}

#endif // __VDS_SERVER_SERVER_JSON_CLIENT_API_H_
