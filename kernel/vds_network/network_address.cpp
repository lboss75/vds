#include "stdafx.h"
#include "network_address.h"
#include "url_parser.h"

vds::network_address::network_address()
: addr_size_(sizeof(this->addr_)) {
  memset((char *)&this->addr_, 0, sizeof(this->addr_));
}

vds::network_address::network_address(sa_family_t af, const std::string &server, uint16_t port) {
  memset((char *)&this->addr_, 0, sizeof(this->addr_));

  switch(af) {
    case AF_INET: {
      auto addr = (sockaddr_in *) &this->addr_;
      addr->sin_family = af;
      addr->sin_port = htons(port);
      addr->sin_addr.s_addr = inet_addr(server.c_str());

      this->addr_size_ = sizeof(sockaddr_in);
      break;
    }
    case AF_INET6: {
      auto addr = (sockaddr_in6 *) &this->addr_;
      addr->sin6_family = af;
      addr->sin6_port = htons(port);
      if(!inet_pton(AF_INET6, 
        (server.empty() || server[0] != '[' || server[server.length() - 1] != ']')
        ? server.c_str()
        : server.substr(1, server.length() - 2).c_str(), &addr->sin6_addr)){
        auto error = errno;
        throw std::system_error(error, std::system_category(), "Convert IPv6 address from text to binary form");
      }


      this->addr_size_ = sizeof(sockaddr_in6);
      break;
    }
    default:
      throw std::runtime_error("Invalid error");
  }
}

std::string vds::network_address::to_string() const {
  return 
    ((AF_INET6 == this->family()) ? "udp6://" : "udp://")
    + this->server() + ":" + std::to_string(this->port());
}

vds::network_address vds::network_address::parse(const std::string& address) {
  vds::network_address result;
  url_parser::parse_addresses(
      address,
      [&result](const std::string &protocol, const std::string &address) -> bool {
        if ("udp" == protocol || "udp6" == protocol) {
          auto na = url_parser::parse_network_address(address);
          if (na.protocol != "udp" && na.protocol != "udp6") {
            throw std::invalid_argument("address");
          }

          result = network_address(
              (na.protocol == "udp") ? AF_INET : AF_INET6,
              na.server,
              (uint16_t) atoi(na.port.c_str()));
        } else {
          throw std::runtime_error("Invalid addresss");
        }
        return true;
      });
  if (!result) {
    throw std::runtime_error("Invalid addresss");
  }
  return result;
}

vds::network_address vds::network_address::parse(sa_family_t family, const std::string& address) {
  vds::network_address result;
  url_parser::parse_addresses(
    address,
    [&result, family](const std::string &protocol, const std::string &address) -> bool {
    if ("udp" == protocol || "udp6" == protocol) {
      auto na = url_parser::parse_network_address(address);
      if (na.protocol != "udp" && na.protocol != "udp6") {
        throw std::invalid_argument("address");
      }
      switch (family) {
        case AF_INET6: {
          if ("udp" == na.protocol) {
            result = network_address(
              AF_INET6,
              ("127.0.0.1" == na.server) ? "::1" : ("::ffff:" + na.server),
              (uint16_t)atoi(na.port.c_str()));
          }
          else if ("udp6" == na.protocol) {
              result = network_address(
                AF_INET6,
                na.server,
                (uint16_t)atoi(na.port.c_str()));
            }
          break;
        }
        case AF_INET: {
          if ("udp" == na.protocol) {
            result = network_address(
              AF_INET,
              na.server,
              (uint16_t)atoi(na.port.c_str()));
          }
          else if ("udp6" == na.protocol) {
            throw std::runtime_error("Invalid addresss");
          }
          break;
        }
      }
    }
    else {
      throw std::runtime_error("Invalid addresss");
    }
    return true;
  });
  if (!result) {
    throw std::runtime_error("Invalid addresss");
  }
  return result;
}

std::string vds::network_address::server() const {
//  char buffer[512];
//
//  auto addr = (sockaddr *)&this->addr_;
//  inet_ntop(addr->sa_family, addr, buffer, this->addr_size_);
//
//  return buffer;
  char buffer[512];

  const auto err = getnameinfo(
      (struct sockaddr *)&this->addr_,
      this->addr_size_,
      buffer,
      sizeof(buffer),
      0,
      0,
      NI_NUMERICHOST);

  if (0 != err) {
    const auto error = errno;
    throw std::system_error(error, std::system_category(), "getnameinfo");
  }

  return buffer;
}

uint16_t vds::network_address::port() const {
  auto addr = (sockaddr *)&this->addr_;
  switch(addr->sa_family) {
    case AF_INET:{
      return ntohs(reinterpret_cast<const sockaddr_in *>(&this->addr_)->sin_port);
    }
    case AF_INET6:{
      return ntohs(reinterpret_cast<const sockaddr_in6 *>(&this->addr_)->sin6_port);
    }
    default: {
      throw std::runtime_error("Invalid error");
    }
  }
}

vds::network_address::network_address(sa_family_t af, uint16_t port) {
  memset((char *)&this->addr_, 0, sizeof(this->addr_));

  switch(af) {
    case AF_INET: {
      auto addr = (sockaddr_in *) &this->addr_;
      addr->sin_family = af;
      addr->sin_port = htons(port);
      addr->sin_addr.s_addr = INADDR_ANY;

      this->addr_size_ = sizeof(sockaddr_in);
      break;
    }
    case AF_INET6: {
      auto addr = (sockaddr_in6 *) &this->addr_;
      addr->sin6_family = af;
      addr->sin6_port = htons(port);
      addr->sin6_addr = IN6ADDR_ANY_INIT;

      this->addr_size_ = sizeof(sockaddr_in6);
      break;
    }
    default:
      throw std::runtime_error("Invalid error");
  }

}

bool vds::network_address::is_martian() const {
  auto addr = (const sockaddr *)&this->addr_;
  switch(addr->sa_family) {
    case AF_INET: {
      struct sockaddr_in *sin = (struct sockaddr_in*)&this->addr_;
      const unsigned char *address = (const unsigned char*)&sin->sin_addr;
      return sin->sin_port == 0 ||
             (address[0] == 0) ||
             (address[0] == 127) ||
             ((address[0] & 0xE0) == 0xE0);
    }
    case AF_INET6: {
      static const unsigned char zeroes[20] = {0};
      static const unsigned char v4prefix[16] = {
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0
      };
      struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&this->addr_;
      const unsigned char *address = (const unsigned char*)&sin6->sin6_addr;
      return sin6->sin6_port == 0 ||
             (address[0] == 0xFF) ||
             (address[0] == 0xFE && (address[1] & 0xC0) == 0x80) ||
             (memcmp(address, zeroes, 15) == 0 &&
              (address[15] == 0 || address[15] == 1)) ||
             (memcmp(address, v4prefix, 12) == 0);
    }
    default:
      throw std::runtime_error("Invalid state");
  }
}


