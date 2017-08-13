#ifndef __VDS_NETWORK_UDP_SOCKET_H_
#define __VDS_NETWORK_UDP_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_stream.h"
#include "const_data_buffer.h"

namespace vds {
  class _udp_socket;
  class _udp_datagram;
  class _udp_server;
  class _udp_client;
  
  class udp_datagram
  {
  public:

    //The maximum safe UDP payload
    static const size_t max_safe_data_size = 32 * 1024;

    udp_datagram();

    udp_datagram(
      const std::string & server,
      uint16_t port,
      const void * data,
      size_t data_size,
      bool check_max_safe_data_size = true);

    udp_datagram(
      const std::string & server,
      uint16_t port,
      const const_data_buffer & data,
      bool check_max_safe_data_size = true);

    //void reset(
    //  const std::string & server,
    //  uint16_t port,
    //  const void * data,
    //  size_t data_size,
    //  bool check_max_safe_data_size = false);

    std::string server() const;
    uint16_t port() const;

    const void * data() const;
    size_t data_size() const;
    
    _udp_datagram * operator -> () const { return this->impl_.get(); }

    udp_datagram & operator = (const udp_datagram & r)
    {
      auto p1 = this->impl_.get();
      auto id1 = this->id_;

      auto p2 = r.impl_.get();
      auto id2 = r.id_;

      if (this->impl_.get() != r.impl_.get()) {
        this->impl_ = r.impl_;
      }

      return *this;
    }

  private:
    friend class _udp_datagram;
    udp_datagram(_udp_datagram * impl);

    int id_;
    std::shared_ptr<_udp_datagram> impl_;
  };

  class udp_socket
  {
  public:
    udp_socket();
    ~udp_socket();

    std::shared_ptr<continuous_stream<udp_datagram>> incoming();
    std::shared_ptr<async_stream<udp_datagram>> outgoing();

    void stop();

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
