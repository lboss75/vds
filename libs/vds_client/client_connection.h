#ifndef __VDS_CLIENT_CLIENT_CONNECTION_H_
#define __VDS_CLIENT_CLIENT_CONNECTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "tcp_network_socket.h"
#include "http_message.h"

namespace vds {

  class client_connection
  {
  public:
    client_connection(
      const std::string & address,
      int port,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key
    );

    ~client_connection();

    enum class STATE
    {
      NONE,
      CONNECTING,
      CONNECTED,
      CONNECT_ERROR
    };

    STATE state() const
    {
      return this->state_;
    }

    const std::chrono::time_point<std::chrono::steady_clock> & connection_start() const
    {
      return this->connection_start_;
    }

    const std::chrono::time_point<std::chrono::steady_clock> & connection_end() const
    {
      return this->connection_end_;
    }
    
    void connect(const service_provider & sp);
    
    std::shared_ptr<async_stream<std::shared_ptr<http_message>>> incoming_stream() const { return this->incoming_stream_; }
    std::shared_ptr<async_stream<std::shared_ptr<http_message>>> outgoing_stream() const { return this->outgoing_stream_; }
   
  private:
    std::string address_;
    int port_;
    certificate * client_certificate_;
    asymmetric_private_key * client_private_key_;
    STATE state_;
    std::shared_ptr<async_stream<std::shared_ptr<http_message>>> incoming_stream_;
    std::shared_ptr<async_stream<std::shared_ptr<http_message>>> outgoing_stream_;

    std::chrono::time_point<std::chrono::steady_clock> connection_start_;
    std::chrono::time_point<std::chrono::steady_clock> connection_end_;
    
    cancellation_token_source cancellation_source_;
  };
}

#endif // __VDS_CLIENT_CLIENT_CONNECTION_H_
