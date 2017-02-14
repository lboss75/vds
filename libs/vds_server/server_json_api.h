#ifndef __VDS_SERVER_SERVER_JSON_API_H_
#define __VDS_SERVER_SERVER_JSON_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_json_api
  {
  public:
    server_json_api(
      const service_provider & sp,
      ssl_peer & peer
    );

    json_value * operator()(json_value * request) const;

  private:
    logger log_;
    ssl_peer & peer_;
  };
}

#endif // __VDS_SERVER_SERVER_JSON_API_H_
