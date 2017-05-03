#ifndef __VDS_CLIENT_CLIENT_LOGIC_H_
#define __VDS_CLIENT_CLIENT_LOGIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client_connection.h"

namespace vds {
  class client_logic
  {
  public:
    client_logic();

    ~client_logic();

    void start(
      const service_provider & sp,
      const std::string & server_address,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key);

    void stop(const service_provider & sp);

    void connected(const service_provider & sp, client_connection<client_logic> & connection);
    void connection_closed(const service_provider & sp, client_connection<client_logic> & connection);
    void connection_error(
      const service_provider & sp,
      client_connection<client_logic> & connection,
      std::exception_ptr ex);

    void process_response(
      const service_provider & sp,
      client_connection<client_logic> & connection,
      const json_value * response);

    void get_commands(
      const service_provider & sp,
      client_connection<client_logic> & connection);

    void add_task(
      const service_provider & sp,
      const std::string & message);

   
    template <typename response_type>
    async_task<const response_type & /*response*/>
    send_request(
      const service_provider & sp,
      std::unique_ptr<json_value> && message,
      const std::chrono::steady_clock::duration & request_timeout = std::chrono::seconds(60))
    {
      std::unique_ptr<json_value> m = std::move(message);
      auto s = dynamic_cast<json_object *>(m.get());
      if(nullptr == s){
        throw std::runtime_error("Invalid argument");
      }
      
      auto request_id = guid::new_guid().str();
      s->add_property("$r", request_id);
      
      auto task = s->json_value::str();
      
      return create_async_task(
        [this, sp, task, request_id, request_timeout](
          const std::function<void (const service_provider & sp, const response_type & response)> & done,
          const error_handler & on_error,
          const service_provider & sp){
          std::lock_guard<std::mutex> lock(this->requests_mutex_);
          this->requests_.set(request_id, request_info {
            task,
            [done](const service_provider & sp, const json_value * response) { done(sp, response_type(response)); },
            on_error });
          this->add_task(sp, task);
          
          auto t = std::make_shared<timer>();
          t->start(sp, request_timeout,
            [this, sp, t, request_id](){
              t->stop(sp);
              this->cancel_request(sp, request_id);              
              return false;
            });
        });
    }
    
    void cancel_request(
      const service_provider & sp,
      const std::string & request_id);

    async_task<const std::string& /*version_id*/> put_file(
      const service_provider & sp,
      const std::string & user_login,
      const std::string & name,
      const const_data_buffer & data);

    async_task<const const_data_buffer & /*datagram*/> download_file(
      const service_provider & sp,
      const std::string & user_login,
      const std::string & name);

  private:
    std::string server_address_;
    certificate * client_certificate_;
    asymmetric_private_key * client_private_key_;

    std::vector<std::unique_ptr<client_connection<client_logic>>> connection_queue_;
    std::future<void> update_connection_pool_feature_;

    static constexpr size_t MAX_CONNECTIONS = 10;
    std::mutex connection_mutex_;
    size_t connected_;
    
    bool messages_sent_;

    pipeline_queue<std::string, client_connection<client_logic>> outgoing_queue_;
    
    timer process_timer_;
    bool process_timer_tasks(const service_provider & sp);
    
    struct request_info
    {
      std::string task;
      std::function<void (const service_provider & sp, const json_value * response)> done;
      error_handler on_error;
    };
    
    std::mutex requests_mutex_;
    simple_cache<std::string, request_info> requests_;    
  };
}


#endif // __VDS_CLIENT_CLIENT_LOGIC_H_
