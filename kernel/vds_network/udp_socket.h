#ifndef __VDS_NETWORK_UDP_SOCKET_H_
#define __VDS_NETWORK_UDP_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_stream.h"

namespace vds {
  class _udp_socket;
  class _udp_datagram;
  class _udp_server;
  class _udp_client;
  
  class udp_datagram
  {
  public:
    udp_datagram();

    udp_datagram(
      const std::string & server,
      uint16_t port,
      const void * data,
      size_t data_size);
    
    void reset(
      const std::string & server,
      uint16_t port,
      const void * data,
      size_t data_size);

    const std::string & server() const;
    uint16_t port() const;

    const void * data() const;
    size_t data_size() const;
    
    _udp_datagram * operator -> () const { return this->impl_.get(); }

  private:
    friend class _udp_datagram;
    udp_datagram(_udp_datagram * impl);

    std::shared_ptr<_udp_datagram> impl_;
  };

  class udp_socket
  {
  public:
    udp_socket();
    ~udp_socket();

    std::shared_ptr<async_stream<udp_datagram>> incoming();
    std::shared_ptr<async_stream<udp_datagram>> outgoing();

    void close();

    _udp_socket * operator -> () const { return this->impl_.get(); }

  private:
    friend class _udp_socket;
    udp_socket(const std::shared_ptr<_udp_socket> & impl);

    std::shared_ptr<_udp_socket> impl_;
  };


  class udp_server
  {
  public:
    udp_server();
    ~udp_server();

    udp_socket start(
      const service_provider & sp,
      const std::string & address,
      int port);

    void stop(const service_provider & sp);

  private:
    std::shared_ptr<_udp_server> impl_;
  };

  class udp_client
  {
  public:
    udp_client();
    ~udp_client();

    udp_socket start(
      const service_provider & sp);

    void stop(const service_provider & sp);

  private:
    std::shared_ptr<_udp_client> impl_;
  };

}

#endif//__VDS_NETWORK_UDP_SOCKET_H_
