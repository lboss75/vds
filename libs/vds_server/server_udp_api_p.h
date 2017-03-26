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
      server_udp_api * owner,
      certificate & certificate,
      asymmetric_private_key & private_key);
    ~_server_udp_api();
      
    void start(const std::string & address, size_t port);
    void stop();

    //UDP server handlers
    void udp_server_done();
    void udp_server_error(std::exception * ex);
    void socket_closed(std::list<std::exception *> errors);
    void input_message(const sockaddr_in & from, const void * data, size_t len);

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
    certificate & certificate_;
    asymmetric_private_key & private_key_;
    udp_socket s_;
    
    event_handler<certificate *> on_download_certificate_;
    void on_download_certificate(certificate * cert);
    
    uint32_t out_session_id_;
    
    pipeline<std::string, uint16_t, data_buffer> message_queue_;

    struct udp_client
    {

    };

    void update_upd_connection_pool();
    
    void send_welcome();
  };
}

#endif // __VDS_SERVER_SERVER_UDP_API_P_H_

