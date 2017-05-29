#ifndef __VDS_CLIENT_CLIENT_CONNECTION_H_
#define __VDS_CLIENT_CLIENT_CONNECTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  template <typename connection_handler_type>
  class client_connection
  {
  public:
    client_connection(
      connection_handler_type * handler,
      const std::string & address,
      int port,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key
    )
    : handler_(handler),
      address_(address),
      port_(port),
      client_certificate_(client_certificate),
      client_private_key_(client_private_key),
      state_(NONE)
    {
    }

    ~client_connection()
    {

    }

    enum connection_state
    {
      NONE,
      CONNECTING,
      CONNECTED,
      CONNECT_ERROR
    };

    connection_state state() const
    {
      return this->state_;
    }

    const std::chrono::time_point<std::chrono::system_clock> & connection_start() const
    {
      return this->connection_start_;
    }

    const std::chrono::time_point<std::chrono::system_clock> & connection_end() const
    {
      return this->connection_end_;
    }
    
    void connect(const service_provider & sp);
   
  private:
    connection_handler_type * handler_;
    std::string address_;
    int port_;
    certificate * client_certificate_;
    asymmetric_private_key * client_private_key_;
    connection_state state_;

    std::chrono::time_point<std::chrono::system_clock> connection_start_;
    std::chrono::time_point<std::chrono::system_clock> connection_end_;
  };
}

#endif // __VDS_CLIENT_CLIENT_CONNECTION_H_
