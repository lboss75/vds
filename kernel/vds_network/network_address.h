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

    bool is_martian() const;

    int compare(const network_address & other) const {
      if(this->addr_.ss_family == other.addr_.ss_family){
        switch (this->addr_.ss_family){
          case AF_INET: {
            const auto result = reinterpret_cast<const sockaddr_in *>(&this->addr_)->sin_addr.s_addr -
                                reinterpret_cast<const sockaddr_in *>(&other.addr_)->sin_addr.s_addr;
            if (0 != result) {
              return result;
            }

            return reinterpret_cast<const sockaddr_in *>(&this->addr_)->sin_port -
                   reinterpret_cast<const sockaddr_in *>(&other.addr_)->sin_port;
          }
          case AF_INET6: {
            const auto l = reinterpret_cast<const sockaddr_in6 *>(&this->addr_);
            const auto r = reinterpret_cast<const sockaddr_in6 *>(&other.addr_);
            auto result = memcmp(
                &l->sin6_addr,
                &r->sin6_addr,
                sizeof(l->sin6_addr));
            if(0 != result){
              return result;
            }

            result = ntohs(l->sin6_port) - ntohs(r->sin6_port);
            if(0 != result){
              return result;
            }

            result = l->sin6_flowinfo - r->sin6_flowinfo;
            if(0 != result){
              return result;
            }

            return l->sin6_scope_id - r->sin6_scope_id;
          }
          default:{
            throw std::runtime_error("Invalid argument");
          }
        }
      }
      else {
        if(this->addr_.ss_family == AF_INET && other.addr_.ss_family == AF_INET6){
          const auto l = reinterpret_cast<const sockaddr_in *>(&this->addr_);
          const auto r = reinterpret_cast<const sockaddr_in6 *>(&other.addr_);
          if(IN6_IS_ADDR_V4MAPPED(&r->sin6_addr)){
#ifdef _WIN32
            const auto result = l->sin_addr.s_addr - r->sin6_addr.u.Word[3];
#else
            const auto result = l->sin_addr.s_addr - r->sin6_addr.s6_addr32[3];
#endif
            if(0 != result){
              return result;
            }

            return l->sin_port - r->sin6_port;
          }
        }
        else
        if(this->addr_.ss_family == AF_INET6 && other.addr_.ss_family == AF_INET){
          const auto l = reinterpret_cast<const sockaddr_in6 *>(&this->addr_);
          const auto r = reinterpret_cast<const sockaddr_in *>(&other.addr_);
          if(IN6_IS_ADDR_V4MAPPED(&l->sin6_addr)){
#ifdef _WIN32
            const auto result = l->sin6_addr.u.Word[3] - r->sin_addr.s_addr;
#else
            const auto result = l->sin6_addr.s6_addr32[3] - r->sin_addr.s_addr;
#endif
            if(0 != result){
              return result;
            }

            return l->sin6_port - r->sin_port;
          }
        }

        return this->addr_.ss_family - other.addr_.ss_family;
      }
    }

    bool operator == (const network_address & other) const {
      return this->compare(other) == 0;
    }

    bool operator < (const network_address & other) const {
      return this->compare(other) < 0;
    }

    bool operator > (const network_address & other) const {
      return this->compare(other) > 0;
    }

  private:
    sockaddr_storage addr_;
    socklen_t addr_size_;
  };
}

#endif //__VDS_NETWORK_NETWORK_ADDRESS_H_
