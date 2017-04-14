#ifndef __VDS_SERVER_SERVER_UDP_API_P_H_
#define __VDS_SERVER_SERVER_UDP_API_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server.h"

namespace vds {
  class server_udp_api;
  class _server_udp_api
  {
  public:
    _server_udp_api(
      const service_provider & sp,
      server_udp_api * owner);
    ~_server_udp_api();
      
    void start(const std::string & address, size_t port);
    void stop();

    //UDP server handlers
    void udp_server_done();
    void udp_server_error(std::exception_ptr ex);
    void socket_closed(std::list<std::exception_ptr> errors);
    async_task<> input_message(const sockaddr_in * from, const void * data, size_t len);

    template <typename next_step_type>
    void get_message(next_step_type & next)
    {
      this->message_queue_.get(next);
    }
    
    void open_udp_session(const std::string & address);


  private:
    service_provider sp_;
    logger log_;
    server_udp_api * const owner_;
    udp_socket s_;
    lazy_service<istorage_log> storage_log_;
    
    class out_session_data
    {
    public:
      out_session_data(
        const std::string & original_server,
        uint16_t original_port);

      void init_session(
        uint32_t external_session_id,
        const std::string & real_server,
        uint16_t real_port,
        certificate && cert,
        symmetric_key && session_key);

    private:
      std::string original_server_;
      uint16_t original_port_;
      uint32_t external_session_id_;
      std::string real_server_;
      uint16_t real_port_;
      std::unique_ptr<certificate> cert_;
      std::unique_ptr<symmetric_key> session_key_;
    };
    std::map<uint32_t, std::unique_ptr<out_session_data>> out_sessions_;
    
    
    class in_session_data
    {
    public:
      in_session_data(
        const guid & server_id,
        const symmetric_key & session_key);

      const symmetric_key & session_key() const { return this->session_key_; }
    private:
      guid server_id_;
      symmetric_key session_key_;
    };
    std::map<uint32_t, std::unique_ptr<in_session_data>> in_sessions_;
    
    pipeline<std::string, uint16_t, const_data_buffer> message_queue_;

    struct udp_client
    {

    };

    void update_upd_connection_pool();
    
    void send_welcome();
  };
}

#endif // __VDS_SERVER_SERVER_UDP_API_P_H_

