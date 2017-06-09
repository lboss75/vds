#ifndef __VDS_CLIENT_CLIENT_LOGIC_H_
#define __VDS_CLIENT_CLIENT_LOGIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <future>
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

    template <typename response_type>
    async_task<const response_type & /*response*/>
    send_request(
      const service_provider & sp,
      const std::shared_ptr<json_value> & message,
      const std::chrono::steady_clock::duration & request_timeout = std::chrono::seconds(60))
    {
      auto s = dynamic_cast<json_object *>(message.get());
      if(nullptr == s){
        throw std::runtime_error("Invalid argument");
      }
      
      auto request_id = guid::new_guid().str();
      s->add_property("$r", request_id);
      
      return create_async_task(
        [this, sp, message, request_id, request_timeout](
          const std::function<void (const service_provider & sp, const response_type & response)> & done,
          const error_handler & on_error,
          const service_provider & sp){
          std::lock_guard<std::mutex> lock(this->requests_mutex_);
          this->requests_.set(request_id, std::make_shared<request_info>(
            message,
            [done, on_error](const service_provider & sp, const std::shared_ptr<json_value> & response) { 
              auto response_object = std::dynamic_pointer_cast<json_object>(response);
              std::string error_message;
              if (response_object && response_object->get_property("$e", error_message)){
                on_error(sp, std::make_shared<std::runtime_error>(error_message));
              }
              else {
                done(sp, response_type(response));
              }
            },
            on_error));
          this->add_task(sp, message);
          
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

  private:
    std::string server_address_;
    certificate * client_certificate_;
    asymmetric_private_key * client_private_key_;

    std::vector<std::unique_ptr<client_connection>> connection_queue_;
    std::future<void> update_connection_pool_feature_;

    static constexpr size_t MAX_CONNECTIONS = 10;
    std::mutex connection_mutex_;
    size_t connected_;
    
    bool messages_sent_;

    //pipeline_queue<std::string, client_connection<client_logic>> outgoing_queue_;
    
    void add_task(
      const service_provider & sp,
      const std::shared_ptr<json_value> & message);

    timer process_timer_;
    bool process_timer_tasks(const service_provider & sp);
    
    class request_info : public std::enable_shared_from_this<request_info>
    {
    public:
      request_info(
        const std::shared_ptr<json_value> & task,
        const std::function<void(const service_provider & sp, const std::shared_ptr<json_value> & response)> & done,
        const error_handler & on_error);

      void done(const service_provider & sp, const std::shared_ptr<json_value> & response);
      void on_timeout(const service_provider & sp);
      std::shared_ptr<json_value> task() const;

    private:
      std::shared_ptr<json_value> task_;
      std::function<void(const service_provider & sp, const std::shared_ptr<json_value> & response)> done_;
      error_handler on_error_;

      mutable std::mutex mutex_;
      bool is_completed_;
    };
    
    std::mutex requests_mutex_;
    simple_cache<std::string, std::shared_ptr<request_info>> requests_;    

    void continue_read_connection(
      const service_provider & sp,
      client_connection * connection,
      std::shared_ptr<std::shared_ptr<http_message>> buffer);

    void process_response(
      const service_provider & sp,
      //client_connection<client_logic> & connection,
      const std::shared_ptr<json_value> & response);

  };
}


#endif // __VDS_CLIENT_CLIENT_LOGIC_H_
