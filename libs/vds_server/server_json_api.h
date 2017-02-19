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
      const service_provider & sp
    );

    json_value * operator()(const service_provider & scope, const json_value * request) const;

  private:
    service_provider sp_;
    logger log_;

    void process(const vsr_new_client_message & message) const;
  };
}

#endif // __VDS_SERVER_SERVER_JSON_API_H_
