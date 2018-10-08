#include "stdafx.h"
#include "udp_socket.h"
#include "private/udp_socket_p.h"

vds::udp_datagram::udp_datagram()
  : impl_(nullptr)
{
}

vds::udp_datagram::udp_datagram(const udp_datagram& other)
: impl_(new _udp_datagram(*other.impl_)){
}

vds::udp_datagram::udp_datagram(udp_datagram && other)
  : impl_(other.impl_) {
  other.impl_ = nullptr;
}

vds::udp_datagram::udp_datagram(vds::_udp_datagram* impl)
  : impl_(impl)
{
}

std::future<vds::udp_datagram> vds::udp_datagram_reader::read_async() {
  return static_cast<_udp_receive *>(this)->read_async();
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

vds::udp_datagram::~udp_datagram() {
  delete this->impl_;
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

vds::udp_datagram& vds::udp_datagram::operator=(const udp_datagram& other) {
  delete this->impl_;
  this->impl_ = new _udp_datagram(*other.impl_);
  return *this;
}

vds::udp_datagram& vds::udp_datagram::operator=(udp_datagram&& other) {
  if (this != &other) {
    delete this->impl_;
    this->impl_ = other.impl_;
    other.impl_ = nullptr;
  }
  return *this;

}

std::future<void> vds::udp_datagram_writer::write_async( const udp_datagram& message) {
  return static_cast<_udp_send *>(this)->write_async(message);
}

vds::udp_socket::udp_socket()
{
}

vds::udp_socket::~udp_socket()
{
  delete this->impl_;
}

std::tuple<std::shared_ptr<vds::udp_datagram_reader>, std::shared_ptr<vds::udp_datagram_writer>>
vds::udp_socket::start(const service_provider * sp)
{
  return {
    std::make_shared<_udp_receive>(sp, this->shared_from_this()),
    std::make_shared<_udp_send>(sp, this->shared_from_this())
  };
}

void vds::udp_socket::stop()
{
}

std::shared_ptr<vds::udp_socket> vds::udp_socket::create(
  const service_provider * sp,
    sa_family_t af)
{
#ifdef _WIN32
  auto s = WSASocket(af, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (INVALID_SOCKET == s) {
    auto error = WSAGetLastError();
    throw std::system_error(error, std::system_category(), "create socket");
  }

  if (af == AF_INET6) {
    int no = 0;
    if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&no, sizeof(no)) < 0) {
      auto error = WSAGetLastError();
      ::close(s);
      throw std::system_error(error, std::system_category(), "set IPV6_V6ONLY=0");
    }
  }
  (*sp->get<network_service>())->associate(s);
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

  if (af == AF_INET6) {
    int no = 0;
    if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&no, sizeof(no)) < 0) {
      auto error = errno;
      ::close(s);
      throw std::system_error(error, std::system_category(), "set IPV6_V6ONLY=0");
    }
  }

#endif
  return std::shared_ptr<udp_socket>(new udp_socket(new _udp_socket(s)));
}

#ifndef _WIN32
void vds::udp_socket::process(uint32_t events) {
  this->impl_->process(events);
}

void vds::_udp_socket::process(uint32_t events) {
  if (EPOLLOUT == (EPOLLOUT & events)) {
    if (0 == (this->event_masks_ & EPOLLOUT)) {
      throw std::runtime_error("Invalid state");
    }

    auto w = this->write_task_.lock();
    if(w) {
      w->process();
    }
  }

  if (EPOLLIN == (EPOLLIN & events)) {
    if (0 == (this->event_masks_ & EPOLLIN)) {
      throw std::runtime_error("Invalid state");
    }

    auto r = this->read_task_.lock();
    if(r){
      r->process();
    }
  }
}
#endif//_WIN32

vds::udp_server::udp_server()
  : impl_(nullptr)
{
}

vds::udp_server::~udp_server()
{
  delete this->impl_;
}

std::tuple<std::shared_ptr<vds::udp_datagram_reader>, std::shared_ptr<vds::udp_datagram_writer>> vds::udp_server::start(
  const service_provider * sp,
  const network_address & address)
{
  vds_assert(nullptr == this->impl_);
  this->impl_ = new _udp_server(address);
  return this->impl_->start(sp);
}

void vds::udp_server::stop()
{
  this->impl_->stop();
  delete this->impl_;
  this->impl_ = nullptr;
}

const std::shared_ptr<vds::udp_socket> &vds::udp_server::socket() const {
  return this->impl_->socket();
}


const vds::network_address& vds::udp_server::address() const {
  return this->impl_->address();
}

void vds::udp_server::prepare_to_stop() {
  this->impl_->prepare_to_stop();
}

vds::udp_client::udp_client()
{
}

vds::udp_client::~udp_client()
{
}

std::tuple<std::shared_ptr<vds::udp_datagram_reader>, std::shared_ptr<vds::udp_datagram_writer>>  vds::udp_client::start(
  const service_provider * sp,
    sa_family_t af) {
  vds_assert(nullptr == this->impl_);
  this->impl_ = new _udp_client();
  return this->impl_->start(sp, af);
}

void vds::udp_client::stop()
{
}
