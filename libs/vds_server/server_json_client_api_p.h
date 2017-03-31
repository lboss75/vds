#ifndef __VDS_SERVER_SERVER_JSON_CLIENT_API_P_H_
#define __VDS_SERVER_SERVER_JSON_CLIENT_API_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_json_client_api;
  class _server_json_client_api
  {
  public:
    _server_json_client_api(
      const service_provider & sp,
      server_json_client_api * owner
    );
    
    json_value * operator()(const service_provider & scope, const json_value * request) const;

  private:
    logger log_;
    server_json_client_api * const owner_;

    void process(const service_provider & scope, json_array * result, const client_messages::certificate_and_key_request & message) const;
    void process(const service_provider & scope, json_array * result, const client_messages::register_server_request & message) const;
    void process(const service_provider & scope, json_array * result, const client_messages::put_file_message & message) const;
  };
}

#endif // __VDS_SERVER_SERVER_JSON_CLIENT_API_P_H_
