#ifndef __VDS_NETWORK_NETWORK_ADDRESS_H_
#define __VDS_NETWORK_NETWORK_ADDRESS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <sys/socket.h>

namespace vds {
  class network_address {
  public:
    network_address();

    network_address(
        sa_family_t af,
        const std::string & server,
        uint16_t port);

    network_address(
        sa_family_t af,
        uint16_t port);

    static network_address ip4(
        const std::string & server,
        uint16_t port){
      return network_address(AF_INET, server, port);
    }

    static network_address ip6(
        const std::string & server,
        uint16_t port){
      return network_address(AF_INET6, server, port);
    }

    static network_address any_ip4(
        uint16_t port){
      return network_address(AF_INET, port);
    }

    static network_address any_ip6(
        uint16_t port){
      return network_address(AF_INET6, port);
    }

    operator const sockaddr * () const {
      return (const sockaddr *)&this->addr_;
    }

    operator sockaddr * () {
      return (sockaddr *)&this->addr_;
    }

    sa_family_t family() const {
      return reinterpret_cast<const sockaddr *>(&this->addr_)->sa_family;
    }

    std::string server() const;
    uint16_t port() const;

    std::string to_string() const;

    socklen_t * size_ptr() {
      return &this->addr_size_;
    }

    void reset() {
      memset(&this->addr_, 0, sizeof(this->addr_));
      this->addr_size_ = sizeof(this->addr_);
    }

    socklen_t size() const {
      return this->addr_size_;
    }

  private:
    sockaddr_storage addr_;
    socklen_t addr_size_;
  };
}

#endif //__VDS_NETWORK_NETWORK_ADDRESS_H_
