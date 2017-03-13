#ifndef __VDS_SERVER_SERVER_JSON_API_P_H_
#define __VDS_SERVER_SERVER_JSON_API_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_json_api;
  class _server_json_api
  {
  public:
    _server_json_api(
      const service_provider & sp,
      server_json_api * owner
    );
    
    json_value * operator()(const service_provider & scope, const json_value * request) const;

  private:
    logger log_;
    server_json_api * const owner_;
  };
}

#endif // __VDS_SERVER_SERVER_JSON_API_P_H_
