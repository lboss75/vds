#ifndef __VDS_NETWORK_NETWORK_ADDRESS_H_
#define __VDS_NETWORK_NETWORK_ADDRESS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <ws2def.h>
#include <WS2tcpip.h>
typedef ADDRESS_FAMILY sa_family_t;
#endif

namespace vds {
  class network_address {
  public:
    network_address();
    network_address(const network_address & other) = default;
    network_address(network_address && other) = default;

    network_address & operator = (const network_address & other) = default;
    network_address & operator = (network_address && other) = default;


    network_address(
        sa_family_t af,
        uint16_t port);

    //static network_address ip4(
    //    const std::string & server,
    //    uint16_t port){
    //  return network_address(AF_INET, server, port);
    //}

    //static network_address ip6(
    //    const std::string & server,
    //    uint16_t port){
    //  return network_address(AF_INET6, server, port);
    //}
    expected<void> reset(
      sa_family_t af,
      int sock_type,
      int ai_flags,
      int ai_protocol,
      const std::string & server,
      uint16_t port);

    static network_address any_ip4(
        uint16_t port){
      return network_address(AF_INET, port);
    }

    static network_address any_ip6(
        uint16_t port){
      return network_address(AF_INET6, port);
    }

    static expected<network_address> tcp_ip4(
      const std::string & server,
      uint16_t port);

    static expected<network_address> tcp_ip6(
      const std::string & server,
      uint16_t port);

    static expected<network_address> udp_ip4(
      const std::string & server,
      uint16_t port);

    static expected<network_address> udp_ip6(
      const std::string & server,
      uint16_t port);


    operator const sockaddr * () const {
      return (const sockaddr *)&this->addr_;
    }

    operator sockaddr * () {
      return (sockaddr *)&this->addr_;
    }

    sa_family_t family() const;

    expected<std::string> server() const;
    uint16_t port() const;

    std::string to_string() const;
    static expected<network_address> parse(const std::string & address);
    static expected<network_address> parse(sa_family_t family, const std::string & address);

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

    bool is_martian() const;

    int compare(const network_address & other) const;

    bool operator == (const network_address & other) const {
      return this->compare(other) == 0;
    }

    bool operator < (const network_address & other) const {
      return this->compare(other) < 0;
    }

    bool operator > (const network_address & other) const {
      return this->compare(other) > 0;
    }

    explicit operator bool () const {
      return this->addr_.ss_family != 0;
    }
    bool operator ! () const {
      return this->addr_.ss_family == 0;
    }

    void clear();

  private:
    sockaddr_storage addr_;
    socklen_t addr_size_;
  };
}

#endif //__VDS_NETWORK_NETWORK_ADDRESS_H_
