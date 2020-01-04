#include "stdafx.h"
#include "network_address.h"
#include "url_parser.h"

vds::network_address::network_address()
: addr_size_(sizeof(this->addr_)) {
  memset((char *)&this->addr_, 0, sizeof(this->addr_));
}

vds::expected<void> vds::network_address::reset(
  sa_family_t af,
  int sock_type,
  int ai_flags,
  int ai_protocol,
  const std::string &server,
  uint16_t port) {
  memset((char *)&this->addr_, 0, sizeof(this->addr_));

  addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = af;
  hints.ai_socktype = sock_type;//SOCK_DGRAM;
  hints.ai_protocol = ai_protocol;// IPPROTO_UDP;
  hints.ai_flags = ai_flags | AI_NUMERICHOST; // AI_NUMERICSERV | AI_ALL | AI_V4MAPPED

  addrinfo * buffer;
  auto status = getaddrinfo(server.c_str(), std::to_string(port).c_str(), &hints, &buffer);
  if (status) {
    hints.ai_flags = ai_flags;
    status = getaddrinfo(server.c_str(), std::to_string(port).c_str(), &hints, &buffer);
    if (status) {
      return vds::make_unexpected<std::system_error>(status, std::system_category(), "Parse address " + server);
    }
  }

  for (auto res = buffer; res != nullptr; res = res->ai_next) {
    if(res->ai_family == af) {
      switch(af) {
      case AF_INET: {
        this->addr_size_ = sizeof(sockaddr_in);
        memcpy(&this->addr_, res->ai_addr, this->addr_size_);
        freeaddrinfo(buffer);
        return expected<void>();
      }
      case AF_INET6: {
        this->addr_size_ = sizeof(sockaddr_in6);
        memcpy(&this->addr_, res->ai_addr, this->addr_size_);
        freeaddrinfo(buffer);
        return  expected<void>();
      }
      }
    }
  }
  freeaddrinfo(buffer);

  return vds::make_unexpected<std::runtime_error>("Unable to resolve " + server);
}

std::string vds::network_address::to_string() const {
  auto s = this->server();

  return 
    ((AF_INET6 == this->family())
      ? "udp6://"
      : "udp://")
    + (s.has_error() ? s.error()->what() : s.value())
    + ":" + std::to_string(this->port());
}

vds::expected<vds::network_address> vds::network_address::parse(const std::string& address) {
  vds::network_address result;
  CHECK_EXPECTED(url_parser::parse_addresses(
      address,
      [&result](const std::string &protocol, const std::string &address) -> expected<bool> {
        if ("udp" == protocol || "udp6" == protocol) {
          GET_EXPECTED(na, url_parser::parse_network_address(address));
          if (na.protocol != "udp" && na.protocol != "udp6") {
            return vds::make_unexpected<std::invalid_argument>("address");
          }

          CHECK_EXPECTED(result.reset(
              (na.protocol == "udp") ? AF_INET : AF_INET6,
              SOCK_DGRAM,
              AI_NUMERICSERV | AI_ALL | AI_V4MAPPED,
              IPPROTO_UDP,
              na.server,
              (uint16_t) atoi(na.port.c_str())));
        } else if ("tcp" == protocol || "tcp6" == protocol) {
          GET_EXPECTED(na, url_parser::parse_network_address(address));
            if (na.protocol != "tcp" && na.protocol != "tcp6") {
              return vds::make_unexpected<std::invalid_argument>("address");
            }

            CHECK_EXPECTED(result.reset(
              (na.protocol == "tcp") ? AF_INET : AF_INET6,
              SOCK_STREAM,
              AI_NUMERICSERV | AI_ALL | AI_V4MAPPED,
              0,
              na.server,
              (uint16_t)atoi(na.port.c_str())));
          }
          else {
            return vds::make_unexpected<std::runtime_error>("Invalid addresss");
        }
        return true;
      }));
  if (!result) {
    return vds::make_unexpected<std::runtime_error>("Invalid address");
  }
  return result;
}

vds::expected<vds::network_address> vds::network_address::parse_server_address(const std::string& server, bool tcp /*= true*/)
{
  auto result = parse((tcp ? "tcp6" : "udp6") + server);
  if (result.has_value()) {
    return std::move(result.value());
  }

  return parse((tcp ? "tcp" : "udp") + server);
}

vds::expected<vds::network_address> vds::network_address::parse(sa_family_t family, const std::string& address) {
  vds::network_address result;
  CHECK_EXPECTED(url_parser::parse_addresses(
    address,
    [&result, family](const std::string &protocol, const std::string &address) -> expected<bool> {
    if ("udp" == protocol || "udp6" == protocol) {
      GET_EXPECTED(na, url_parser::parse_network_address(address));
      if (na.protocol != "udp" && na.protocol != "udp6") {
        return vds::make_unexpected<std::invalid_argument>("address");
      }
      switch (family) {
        case AF_INET6: {
          if ("udp" == na.protocol) {
            CHECK_EXPECTED(result.reset(
              AF_INET6,
              SOCK_DGRAM,
              AI_NUMERICSERV | AI_ALL | AI_V4MAPPED,
              IPPROTO_UDP,
              na.server,
              (uint16_t)atoi(na.port.c_str())));
          }
          else if ("udp6" == na.protocol) {
            CHECK_EXPECTED(result.reset(
              AF_INET6,
                SOCK_DGRAM,
                AI_NUMERICSERV | AI_ALL | AI_V4MAPPED,
                IPPROTO_UDP,
                na.server,
                (uint16_t)atoi(na.port.c_str())));
            }
          break;
        }
        case AF_INET: {
          if ("udp" == na.protocol) {
            CHECK_EXPECTED(result.reset(
              AF_INET,
              SOCK_DGRAM,
              AI_NUMERICSERV | AI_ALL | AI_V4MAPPED,
              IPPROTO_UDP,
              na.server,
              (uint16_t)atoi(na.port.c_str())));
          }
          else if ("udp6" == na.protocol) {
            return vds::make_unexpected<std::runtime_error>("Invalid addresss");
          }
          break;
        }
      }
    }
    else {
      return vds::make_unexpected<std::runtime_error>("Invalid addresss");
    }
    return true;
  }));
  if (!result) {
    return vds::make_unexpected<std::runtime_error>("Invalid address");
  }
  return result;
}

vds::expected<vds::network_address> vds::network_address::tcp_ip4(const std::string & server, uint16_t port)
{
  vds::network_address result;
  CHECK_EXPECTED(result.reset(AF_INET, SOCK_STREAM, AI_NUMERICSERV | AI_ALL | AI_V4MAPPED, 0, server, port));
  return result;
}

vds::expected<vds::network_address> vds::network_address::tcp_ip6(const std::string & server, uint16_t port)
{
  vds::network_address result;
  CHECK_EXPECTED(result.reset(AF_INET6, SOCK_STREAM, AI_NUMERICSERV | AI_ALL | AI_V4MAPPED, 0, server, port));
  return result;
}

vds::expected<vds::network_address> vds::network_address::udp_ip4(const std::string & server, uint16_t port)
{
  vds::network_address result;
  CHECK_EXPECTED(result.reset(AF_INET, SOCK_DGRAM, AI_NUMERICSERV | AI_ALL | AI_V4MAPPED, IPPROTO_UDP, server, port));
  return result;
}

vds::expected<vds::network_address> vds::network_address::udp_ip6(const std::string & server, uint16_t port)
{
  vds::network_address result;
  CHECK_EXPECTED(result.reset(AF_INET6, SOCK_DGRAM, AI_NUMERICSERV | AI_ALL | AI_V4MAPPED, IPPROTO_UDP, server, port));
  return result;
}

sa_family_t vds::network_address::family() const {
  return reinterpret_cast<const sockaddr *>(&this->addr_)->sa_family;
}

vds::expected<std::string> vds::network_address::server() const {
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
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "getnameinfo");
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
      vds_assert(false);
      return -1;
    }
  }
}

vds::network_address::network_address(
  sa_family_t af,
  uint16_t port) {
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
      vds_assert(false);
    break;
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
      vds_assert(false);
      break;
  }

  return false;
}

int vds::network_address::compare(const network_address & other) const {
  if (this->addr_.ss_family == other.addr_.ss_family) {
    switch (this->addr_.ss_family) {
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
      if (0 != result) {
        return result;
      }

      result = ntohs(l->sin6_port) - ntohs(r->sin6_port);
      if (0 != result) {
        return result;
      }

      result = l->sin6_flowinfo - r->sin6_flowinfo;
      if (0 != result) {
        return result;
      }

      return l->sin6_scope_id - r->sin6_scope_id;
    }
    default: {
      vds_assert(false);
      break;
    }
    }
  }
  else {
    if (this->addr_.ss_family == AF_INET && other.addr_.ss_family == AF_INET6) {
      const auto l = reinterpret_cast<const sockaddr_in *>(&this->addr_);
      const auto r = reinterpret_cast<const sockaddr_in6 *>(&other.addr_);
      if (IN6_IS_ADDR_V4MAPPED(&r->sin6_addr)) {
#ifdef _WIN32
        const auto result = l->sin_addr.s_addr - r->sin6_addr.u.Word[3];
#else
        const auto result = l->sin_addr.s_addr - r->sin6_addr.s6_addr32[3];
#endif
        if (0 != result) {
          return result;
        }

        return l->sin_port - r->sin6_port;
      }
    }
    else
      if (this->addr_.ss_family == AF_INET6 && other.addr_.ss_family == AF_INET) {
        const auto l = reinterpret_cast<const sockaddr_in6 *>(&this->addr_);
        const auto r = reinterpret_cast<const sockaddr_in *>(&other.addr_);
        if (IN6_IS_ADDR_V4MAPPED(&l->sin6_addr)) {
#ifdef _WIN32
          const auto result = l->sin6_addr.u.Word[3] - r->sin_addr.s_addr;
#else
          const auto result = l->sin6_addr.s6_addr32[3] - r->sin_addr.s_addr;
#endif
          if (0 != result) {
            return result;
          }

          return l->sin6_port - r->sin_port;
        }
      }

    return this->addr_.ss_family - other.addr_.ss_family;
  }

  return -1;
}

void vds::network_address::clear() {
  this->addr_size_ = sizeof(this->addr_);
  memset(&this->addr_, 0, sizeof(this->addr_));
}


