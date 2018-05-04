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
  const network_address & address,
  const void* data,
  size_t data_size,
  bool check_max_safe_data_size /*= true*/)
: impl_(new _udp_datagram(address, data, data_size))
{
  if (check_max_safe_data_size && max_safe_data_size < data_size) {
    throw std::runtime_error("Data too long");
  }
}

vds::udp_datagram::udp_datagram(
  const network_address & address,
  const const_data_buffer & data,
  bool check_max_safe_data_size /*= true*/)
  : impl_(new _udp_datagram(address, data))
{
  if (check_max_safe_data_size && max_safe_data_size < data.size()) {
    auto size = data.size();
    throw std::runtime_error(string_format("Data too long: %d, max: %d", size, max_safe_data_size));
  }
}

vds::network_address vds::udp_datagram::address() const {
  return this->impl_->address();
}


//void vds::udp_datagram::reset(const std::string & server, uint16_t port, const void * data, size_t data_size, bool check_max_safe_data_size /*= true*/)
//{
//  if (check_max_safe_data_size && max_safe_data_size < data_size) {
//    throw std::runtime_error("Data too long");
//  }
//
//  this->impl_.reset(new _udp_datagram(server, port, data, data_size));
//}

const uint8_t * vds::udp_datagram::data() const
{
  return this->impl_->data();
}

size_t vds::udp_datagram::data_size() const
{
  return this->impl_ ? this->impl_->data_size() : 0;
}

vds::udp_socket::udp_socket()
{
}

vds::udp_socket::~udp_socket()
{
	std::cout << "vds::udp_socket::~udp_socket\n";
}

vds::async_task<const vds::udp_datagram&> vds::udp_socket::read_async() const
{
  return this->impl_->read_async();
}

vds::async_task<> vds::udp_socket::write_async(const udp_datagram & message) const
{
  return this->impl_->write_async(message);
}

void vds::udp_socket::stop()
{
  this->impl_->stop();
}

vds::udp_socket vds::udp_socket::create(
    const service_provider & sp,
    sa_family_t af)
{
#ifdef _WIN32
  auto s = WSASocket(af, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (INVALID_SOCKET == s) {
    auto error = WSAGetLastError();
    throw std::system_error(error, std::system_category(), "create socket");
  }

  static_cast<_network_service *>(sp.get<inetwork_service>())->associate(s);
#else
  auto s = socket(af, SOCK_DGRAM, IPPROTO_UDP);
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
	std::cout << "vds::udp_server::~udp_server\n";
}

vds::udp_socket & vds::udp_server::start(
  const service_provider & sp,
  const network_address & address)
{
  this->impl_.reset(new _udp_server(address));
  return this->impl_->start(sp);

}

void vds::udp_server::stop(const service_provider & sp)
{
  this->impl_->stop(sp);
  this->impl_.reset();
}

vds::udp_socket &vds::udp_server::socket() {
  return this->impl_->socket();
}

const vds::udp_socket &vds::udp_server::socket() const{
	return this->impl_->socket();
}

void vds::udp_server::prepare_to_stop(const service_provider & sp) {
  this->impl_->prepare_to_stop(sp);
}

vds::udp_client::udp_client()
{
}

vds::udp_client::~udp_client()
{
	std::cout << "vds::udp_client::~udp_client\n";
}

vds::udp_socket & vds::udp_client::start(
    const service_provider & sp,
    sa_family_t af)
{
  this->impl_.reset(new _udp_client());
  return this->impl_->start(sp, af);
}

void vds::udp_client::stop(const service_provider & sp)
{
}
