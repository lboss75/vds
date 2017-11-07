#include "stdafx.h"
#include "udp_socket.h"
#include "private/udp_socket_p.h"

vds::udp_datagram::udp_datagram()
{
}

vds::udp_datagram::udp_datagram(vds::_udp_datagram* impl)
  : impl_(impl)
{
}

vds::udp_datagram::udp_datagram(
  const std::string& server,
  uint16_t port,
  const void* data,
  size_t data_size,
  bool check_max_safe_data_size /*= true*/)
: impl_(new _udp_datagram(server, port, data, data_size))
{
  if (check_max_safe_data_size && max_safe_data_size < data_size) {
    throw std::runtime_error("Data too long");
  }
}

vds::udp_datagram::udp_datagram(
  const std::string& server,
  uint16_t port,
  const const_data_buffer & data,
  bool check_max_safe_data_size /*= true*/)
  : impl_(new _udp_datagram(server, port, data))
{
  if (check_max_safe_data_size && max_safe_data_size < data.size()) {
    auto size = data.size();
    throw std::runtime_error(string_format("Data too long: %d, max: %d", size, max_safe_data_size));
  }
}


//void vds::udp_datagram::reset(const std::string & server, uint16_t port, const void * data, size_t data_size, bool check_max_safe_data_size /*= true*/)
//{
//  if (check_max_safe_data_size && max_safe_data_size < data_size) {
//    throw std::runtime_error("Data too long");
//  }
//
//  this->impl_.reset(new _udp_datagram(server, port, data, data_size));
//}

std::string vds::udp_datagram::server() const
{
  return this->impl_->server();
}

uint16_t vds::udp_datagram::port() const
{
  return this->impl_->port();
}

const void * vds::udp_datagram::data() const
{
  return this->impl_->data();
}

size_t vds::udp_datagram::data_size() const
{
  return this->impl_->data_size();
}

vds::udp_socket::udp_socket()
{
}

vds::udp_socket::~udp_socket()
{
}

void vds::udp_socket::stop()
{
  this->impl_->stop();
}

vds::udp_socket vds::udp_socket::create(const service_provider & sp)
{
#ifdef _WIN32
  auto s = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (INVALID_SOCKET == s) {
    auto error = WSAGetLastError();
    throw std::system_error(error, std::system_category(), "create socket");
  }

  static_cast<_network_service *>(sp.get<inetwork_service>())->associate(s);
#else
  auto s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (0 > s) {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "create socket");
  }

  /*************************************************************/
  /* Allow socket descriptor to be reuseable                   */
  /*************************************************************/
  int on = 1;
  if (0 > setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
    auto error = errno;
    ::close(s);
    throw std::system_error(error, std::system_category(), "Allow socket descriptor to be reuseable");
  }

  /*************************************************************/
  /* Set socket to be nonblocking. All of the sockets for    */
  /* the incoming connections will also be nonblocking since  */
  /* they will inherit that state from the listening socket.   */
  /*************************************************************/
  if (0 > ioctl(s, FIONBIO, (char *)&on)) {
    auto error = errno;
    ::close(s);
    throw std::system_error(error, std::system_category(), "Set socket to be nonblocking");
  }
#endif
  return udp_socket(std::make_shared<_udp_socket>(s));
}

vds::udp_server::udp_server()
{
}

vds::udp_server::~udp_server()
{
}

vds::udp_socket vds::udp_server::start(
  const service_provider & sp,
  const std::string & address,
  int port,
  const std::function<void(const udp_datagram &)> & target)
{
  this->impl_.reset(new _udp_server(address, port));
  return this->impl_->start(sp, target);

}

void vds::udp_server::stop(const service_provider & sp)
{
  this->impl_->stop(sp);
}

vds::udp_client::udp_client()
{
}

vds::udp_client::~udp_client()
{
}

vds::udp_socket vds::udp_client::start(
    const service_provider & sp,
    const std::function<void(const udp_datagram &)> & target)
{
  this->impl_.reset(new _udp_client());
  return this->impl_->start(sp, target);
}

void vds::udp_client::stop(const service_provider & sp)
{
}
