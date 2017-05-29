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
      server_json_client_api * owner
    );
    
    std::shared_ptr<json_value> operator()(const service_provider & scope, const std::shared_ptr<json_value> & request);

  private:
    server_json_client_api * const owner_;
    
    struct task_info
    {
      std::shared_ptr<json_value> result;
      std::exception_ptr error;
    };
    
    std::mutex task_mutex_;
    simple_cache<std::string, task_info> tasks_;

    vds::async_task<std::shared_ptr<json_value>> process(const vds::service_provider& sp, const vds::client_messages::certificate_and_key_request& message);
    async_task<std::shared_ptr<json_value>> process(const service_provider & scope, const client_messages::register_server_request & message);
    async_task<std::shared_ptr<json_value>> process(const service_provider & scope, const client_messages::put_file_message & message);
    async_task<std::shared_ptr<json_value>> process(const service_provider & scope, const client_messages::get_file_message_request & message);
  };
}

#endif // __VDS_SERVER_SERVER_JSON_CLIENT_API_P_H_
