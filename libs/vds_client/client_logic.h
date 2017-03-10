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
    client_logic(
      const service_provider & sp,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key
    );

    ~client_logic();

    void start();
    void stop();

    void connected(client_connection<client_logic> & connection);
    void connection_closed(client_connection<client_logic> & connection);
    void connection_error(client_connection<client_logic> & connection, std::exception * ex);

    void process_response(client_connection<client_logic> & connection, const json_value * response);

    void get_commands(client_connection<client_logic> & connection);

    void add_task(const std::string & message);

    template<typename _Rep, typename _Period>
    bool wait_for(
      const std::chrono::duration<_Rep, _Period> & period,
      const std::string & task_type,
      const std::function<bool(const json_object * value)> & filter) {

      int index;
      barrier b;
      {
        std::unique_lock<std::mutex> lock(this->filters_mutex_);
        index = this->filter_last_index_++;
        try {
          this->filters_[task_type][index] = [&filter, &b](const json_object * value) -> bool {
            if (filter(value)) {
              b.set();
              return true;
            }
            else {
              return false;
            }
          };
        }
        catch (...) {
          this->filters_[task_type].erase(index);
          throw;
        }
      }

      auto result = b.wait_for(period);

      std::unique_lock<std::mutex> lock(this->filters_mutex_);
      this->filters_[task_type].erase(index);

      return result;
    }

    template <typename response_type>
    bool add_task_and_wait(
      const std::string & request,
      const std::function<bool(const response_type & response)> & filter) {
      for(size_t try_count = 0; try_count < 4; ++try_count){
        this->add_task(request);
  
        if (this->wait_for(
          std::chrono::seconds(5),
          response_type::message_type,
          [filter](const json_object * value) -> bool {
            response_type message(value);
            return filter(message);
          })){
          return true;
        }
      }
      
      return false;
    }

  private:
    service_provider sp_;
    logger log_;
    certificate * client_certificate_;
    asymmetric_private_key * client_private_key_;
    static const char * endpoints_[];

    std::mutex filters_mutex_;
    size_t filter_last_index_;
    std::map<std::string, std::map<size_t, std::function<bool(const json_object * value)>>> filters_;

    std::vector<std::unique_ptr<client_connection<client_logic>>> connection_queue_;
    std::future<void> update_connection_pool_feature_;

    static constexpr size_t MAX_CONNECTIONS = 10;
    std::mutex connection_mutex_;
    size_t connected_;
    
    event_handler<> update_connection_pool_task_;

    pipeline_queue<std::string, client_connection<client_logic>> outgoing_queue_;
    
    void update_connection_pool();
  };
}


#endif // __VDS_CLIENT_CLIENT_LOGIC_H_
