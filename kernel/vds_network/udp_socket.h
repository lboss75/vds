#ifndef __VDS_NETWORK_UDP_SOCKET_H_
#define __VDS_NETWORK_UDP_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_buffer.h"
#include "const_data_buffer.h"
#include "network_address.h"

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
      const network_address & address,
      const void * data,
      size_t data_size,
      bool check_max_safe_data_size = true);

    udp_datagram(
      const network_address & address,
      const const_data_buffer & data,
      bool check_max_safe_data_size = true);

    network_address address() const;

    const uint8_t * data() const;
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
    udp_socket(udp_socket && original);
    udp_socket(const udp_socket & original) = default;

    ~udp_socket();

    udp_socket &operator = (const udp_socket & original) = delete;
    udp_socket & operator = (udp_socket && original) = default;

    std::future<const udp_datagram &> read_async() const;
    std::future<void> write_async(const udp_datagram & message) const;

    void stop();

    _udp_socket * operator -> () const { return this->impl_.get(); }

    static udp_socket create(const service_provider & sp, sa_family_t af);

    operator bool () const {
      return this->impl_.get() != nullptr;
    }

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
      const network_address & address);

    void prepare_to_stop(const service_provider & sp);
    void stop(const service_provider & sp);

    udp_socket & socket();
	  const udp_socket & socket() const;

    _udp_server *operator ->()const {
      return this->impl_.get();
    }
    operator bool () const  {
      return nullptr != this->impl_.get();
    }

    const network_address & address() const;

	bool operator ! () const {
		return nullptr == this->impl_.get() || !this->socket();
	}
  private:
    std::shared_ptr<_udp_server> impl_;
  };

  class udp_client
  {
  public:
    udp_client();
    ~udp_client();

    udp_socket & start(
      const service_provider & sp,
      sa_family_t af);

    void stop(const service_provider & sp);

  private:
    std::shared_ptr<_udp_client> impl_;
  };

}

#endif//__VDS_NETWORK_UDP_SOCKET_H_
