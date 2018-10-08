/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "tcp_network_socket.h"
#include "private/tcp_network_socket_p.h"
#include "private/read_socket_task_p.h"
#include "private/write_socket_task_p.h"

vds::tcp_network_socket::tcp_network_socket()
: impl_(new _tcp_network_socket())
{
}

vds::tcp_network_socket::tcp_network_socket(_tcp_network_socket * impl)
: impl_(impl)
{
}

vds::tcp_network_socket::~tcp_network_socket(){
#ifdef _WIN32
  delete this->impl_;
#endif//_WIN32
}

std::shared_ptr<vds::tcp_network_socket> vds::tcp_network_socket::connect(
  const service_provider * sp,
  const network_address & address)
{

      auto s = std::make_unique<_tcp_network_socket>(
#ifdef _WIN32
        WSASocket(address.family(), SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)
#else
        socket(address.family(), SOCK_STREAM, 0)
#endif//_WIN32
        );

      // Connexion setting for local connexion 
#ifdef _WIN32
      // Connect 
      if (SOCKET_ERROR == ::connect(s->handle(), address, address.size())) {
        // As we are in non-blocking mode we'll always have the error 
        // WSAEWOULDBLOCK whichis actually not one 
        auto error = WSAGetLastError();
        if (WSAEWOULDBLOCK != error) {
          throw std::system_error(error, std::system_category(), "connect");
        }
      }

      (*sp->get<network_service>())->associate(s->handle());
#else
      // Connect 
      if (0 > ::connect(s->handle(), address, address.size())) {
        auto error = errno;
        throw std::system_error(error, std::generic_category(), "connect");
      }
      s->make_socket_non_blocking();
      s->set_timeouts();
#endif
      
      return std::shared_ptr<tcp_network_socket>(new tcp_network_socket(s.release()));
}


void vds::tcp_network_socket::close()
{
  this->impl_->close();
}

#ifdef _WIN32
std::tuple<
  std::shared_ptr<vds::stream_input_async<uint8_t>>,
  std::shared_ptr<vds::stream_output_async<uint8_t>>> vds::tcp_network_socket::start(
  const vds::service_provider *sp) {
  auto pthis = this->shared_from_this();
  return {std::make_shared< _read_socket_task>(sp, pthis), std::make_shared< _write_socket_task>(sp, pthis) };
}

#else//_WIN32

std::tuple<
    std::shared_ptr<vds::stream_input_async<uint8_t>>,
    std::shared_ptr<vds::stream_output_async<uint8_t>>> vds::tcp_network_socket::start(
    const vds::service_provider *sp) {

  auto pthis = this->shared_from_this();
  auto reader = std::make_shared<_read_socket_task>(sp, pthis);
  auto writer = std::make_shared<_write_socket_task>(sp, pthis);

  (*this)->read_task_ = reader;
  (*this)->write_task_ = writer;

  return std::make_tuple(reader, writer);
}

//: network_service_(sp->get<network_service>()->operator->()),
//event_masks_(EPOLLET) {
//}

void vds::tcp_network_socket::process(uint32_t events) {
  this->impl_->process(events);
}

void vds::_tcp_network_socket::process(uint32_t events) {
  if(EPOLLOUT == (EPOLLOUT & events)){
    if(0 == (this->event_masks_ & EPOLLOUT)) {
      throw std::runtime_error("Invalid state");
    }

    this->write_task_.lock()->process();
  }

  if(EPOLLIN == (EPOLLIN & events)){
    if(0 == (this->event_masks_ & EPOLLIN)) {
      throw std::runtime_error("Invalid state");
    }

    this->read_task_.lock()->process();
  }
}

#endif
