/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "tcp_network_socket.h"
#include "private/tcp_network_socket_p.h"

//vds::tcp_network_socket::tcp_network_socket()
//: impl_(new _tcp_network_socket())
//{
//}

vds::tcp_network_socket::tcp_network_socket(_tcp_network_socket * impl)
: impl_(impl)
{
}

vds::tcp_network_socket::~tcp_network_socket(){
#ifdef _WIN32
  delete this->impl_;
#endif//_WIN32
}
vds::expected<std::shared_ptr<vds::tcp_network_socket>> vds::tcp_network_socket::connect(
  const service_provider * sp,
  const network_address & address)
{

      auto s = std::make_unique<_tcp_network_socket>(
#ifdef _WIN32
        WSASocket(address.family(), SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)
#else
        sp,
        socket(address.family(), SOCK_STREAM, 0)
#endif//_WIN32
        );

      // Connexion setting for local connexion 
#ifdef _WIN32
      GET_EXPECTED(handle, s->handle());
      // Connect 
      if (SOCKET_ERROR == ::connect(handle, address, address.size())) {
        // As we are in non-blocking mode we'll always have the error 
        // WSAEWOULDBLOCK whichis actually not one 
        auto error = WSAGetLastError();
        if (WSAEWOULDBLOCK != error) {
          return vds::make_unexpected<std::system_error>(error, std::system_category(), "connect");
        }
      }
      CHECK_EXPECTED((*sp->get<network_service>())->associate(handle));
#else
      // Connect
      GET_EXPECTED(handle, s->handle());
      if (0 > ::connect(handle, address, address.size())) {
        auto error = errno;
        return vds::make_unexpected<std::system_error>(error, std::generic_category(), "connect");
      }
      CHECK_EXPECTED(s->make_socket_non_blocking());
      CHECK_EXPECTED(s->set_timeouts());
#endif
      
      return std::shared_ptr<tcp_network_socket>(new tcp_network_socket(s.release()));
}


vds::expected<void> vds::tcp_network_socket::close()
{
  return this->impl_->close();
}

#ifdef _WIN32
vds::expected<std::tuple<
  std::shared_ptr<vds::stream_input_async<uint8_t>>,
  std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::tcp_network_socket::start(
  const vds::service_provider *sp) {
  auto pthis = this->shared_from_this();
  return {std::make_shared< _read_socket_task>(sp, pthis), std::make_shared< _write_socket_task>(sp, pthis) };
}

#else//_WIN32

vds::expected<
std::tuple<
    std::shared_ptr<vds::stream_input_async<uint8_t>>,
    std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::tcp_network_socket::start(
    const vds::service_provider *sp) {

  auto pthis = this->shared_from_this();
  auto reader = std::make_shared<_read_socket_task>(sp, pthis);
  auto writer = std::make_shared<_write_socket_task>(pthis);

  (*this)->read_task_ = reader;
  (*this)->write_task_ = writer;

  CHECK_EXPECTED(reader->start(sp));

  return std::make_tuple(reader, writer);
}

//: network_service_(sp->get<network_service>()->operator->()),
//event_masks_(EPOLLET) {
//}

vds::expected<void> vds::tcp_network_socket::process(uint32_t events) {
  return this->impl_->process(events);
}

bool vds::tcp_network_socket::operator!() const {
    return !(*this->impl_);
}

vds::expected<void> vds::_tcp_network_socket::process(uint32_t events) {
  if(EPOLLOUT == (EPOLLOUT & events)){
    if(0 == (this->event_masks_ & EPOLLOUT)) {
      return vds::make_unexpected<std::runtime_error>("Invalid state");
    }

    CHECK_EXPECTED(this->write_task_.lock()->process());
  }

  if(EPOLLIN == (EPOLLIN & events)){
    if(0 == (this->event_masks_ & EPOLLIN)) {
      return vds::make_unexpected<std::runtime_error>("Invalid state");
    }

    CHECK_EXPECTED(this->read_task_.lock()->process());
  }

  if(0 != ((EPOLLRDHUP | EPOLLHUP | EPOLLERR) & events)){
    auto pthis = this->read_task_.lock();
    if(pthis){
      CHECK_EXPECTED(pthis->close_read());
    }
  }

  return expected<void>();
}

#endif

vds::expected<void> vds::_tcp_network_socket::close()
{
#ifdef _WIN32
  if (INVALID_SOCKET != this->s_) {
    shutdown(this->s_, SD_BOTH);
    closesocket(this->s_);
    this->s_ = INVALID_SOCKET;
  }
#else
  if (0 <= this->s_) {
    auto r = this->read_task_.lock();
    if(r){
      CHECK_EXPECTED(r->close_read());
    }

    auto w = this->write_task_.lock();
    if(w) {
      CHECK_EXPECTED(w->close_write());
    }
    if (0 != this->event_masks_){
      this->event_masks_ = 0;
      CHECK_EXPECTED((*this->sp_->get<network_service>())->remove_association(this->s_));
    }

    shutdown(this->s_, 2);
    ::close(this->s_);
    this->s_ = -1;
  }
#endif
  return expected<void>();
}
