#ifndef __VDS_NETWORK_UDP_SOCKET_H_
#define __VDS_NETWORK_UDP_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_buffer.h"
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
    static const size_t max_safe_data_size = 508;

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

    const uint8_t * data() const;
    size_t data_size() const;
    
    _udp_datagram * operator -> () const { return this->impl_.get(); }

    udp_datagram & operator = (const udp_datagram & r)
    {
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
    udp_socket(udp_socket && original);
    udp_socket(const udp_socket & original) = delete;

    ~udp_socket();

    udp_socket &operator = (const udp_socket & original) = delete;
    udp_socket & operator = (udp_socket && original) = default;

    async_task<const udp_datagram &> read_async();
    async_task<> write_async(const udp_datagram & message);

    void stop();

    _udp_socket * operator -> () const { return this->impl_.get(); }

    static udp_socket create(const service_provider & sp);

    void send_broadcast(int port, const const_data_buffer &message);

  private:
    friend class _udp_socket;
    udp_socket(const std::shared_ptr<_udp_socket> & impl)
      : impl_(impl)
    {
    }

    std::shared_ptr<_udp_socket> impl_;
  };


  class udp_server
  {
  public:
    udp_server();
    ~udp_server();

    udp_socket & start(
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

    udp_socket & start(
      const service_provider & sp);

    void stop(const service_provider & sp);

  private:
    std::shared_ptr<_udp_client> impl_;
  };

}

#endif//__VDS_NETWORK_UDP_SOCKET_H_
