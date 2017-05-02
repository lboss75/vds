#ifndef __VDS_SERVER_SERVER_JSON_API_P_H_
#define __VDS_SERVER_SERVER_JSON_API_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_json_api;
  class _server_json_api : public server_json_api
  {
  public:
    json_value * operator()(const service_provider & scope, const json_value * request) const;

  };
}

#endif // __VDS_SERVER_SERVER_JSON_API_P_H_
