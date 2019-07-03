/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "tcp_socket_server.h"
#include "private/tcp_socket_server_p.h"

vds::tcp_socket_server::tcp_socket_server()
  : impl_(new _tcp_socket_server())
{
}

vds::tcp_socket_server::~tcp_socket_server()
{
}

vds::expected<void> vds::tcp_socket_server::start(
  const service_provider * sp,
  const network_address & address,
  lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, std::shared_ptr<tcp_network_socket>> new_connection)
{
  return this->impl_->start(sp, address, std::move(new_connection));
}

void vds::tcp_socket_server::stop()
{
  this->impl_->stop();
}


//
vds::_tcp_socket_server::_tcp_socket_server()
  : s_(INVALID_SOCKET)
#ifndef _WIN32
  , is_shuting_down_(false)
#endif
{
}

vds::_tcp_socket_server::~_tcp_socket_server()
{
#ifndef _WIN32
  this->is_shuting_down_ = true;
  close(this->s_);
#else
  closesocket(this->s_);
#endif
  if (this->wait_accept_task_.joinable()) {
    this->wait_accept_task_.join();
  }
}

vds::expected<void> vds::_tcp_socket_server::start(
    const service_provider * sp,
    const network_address & address,
    lambda_holder_t<
      vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>,
      std::shared_ptr<tcp_network_socket>> new_connection) {

#ifdef _WIN32
  this->s_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

  if (INVALID_SOCKET == this->s_) {
    const auto error = WSAGetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "create socket");
  }

  if (SOCKET_ERROR == ::bind(this->s_, address, address.size())) {
    const auto error = WSAGetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "bind");
  }

  if (SOCKET_ERROR == ::listen(this->s_, SOMAXCONN)) {
    const auto error = WSAGetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "listen socket");
  }

  auto accept_result = this->accept_event_.select(this->s_, FD_ACCEPT);
  if (accept_result.has_error()) {
    return vds::unexpected(std::move(accept_result.error()));
  }

  this->wait_accept_task_ = std::thread(
    [this, sp, c = std::move(new_connection)]() {

    HANDLE events[2];
    events[0] = sp->get_shutdown_event().windows_handle();
    events[1] = this->accept_event_.handle();
    for (;;) {
      auto result = WSAWaitForMultipleEvents(2, events, FALSE, INFINITE, FALSE);
      if ((WAIT_OBJECT_0 + 1) != result) {
        break;
      }
      WSANETWORKEVENTS WSAEvents;
      WSAEnumNetworkEvents(
        this->s_,
        this->accept_event_.handle(),
        &WSAEvents);
      if ((WSAEvents.lNetworkEvents & FD_ACCEPT)
        && (0 == WSAEvents.iErrorCode[FD_ACCEPT_BIT])) {
        //Process it
        sockaddr_in client_address;
        int client_address_length = sizeof(client_address);

        auto socket = accept(this->s_, (sockaddr*)&client_address, &client_address_length);
        if (INVALID_SOCKET != socket) {
          sp->get<logger>()->trace("TCP", "Connection from %s", network_service::to_string(client_address).c_str());
          if (!(*sp->get<network_service>())->associate(socket).has_error()) {
            auto s = _tcp_network_socket::from_handle(socket);
            c(s).then([sp, s](vds::expected<std::shared_ptr<stream_output_async<uint8_t>>> result) {
              if (result.has_value() && result.value()) {
                auto handler = std::make_shared<_read_socket_task>(sp, s, std::move(result.value()));
                (void)handler->start();
              }
              else
              {
                (void)s->close();
              }
            });
          }
        }
      }
    }
  });
#else
  this->s_ = socket(AF_INET, SOCK_STREAM, 0);
  if (this->s_ < 0) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category());
  }

  /*************************************************************/
  /* Allow socket descriptor to be reuseable                   */
  /*************************************************************/
  int on = 1;
  if (0 > setsockopt(this->s_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category());
  }

  /*************************************************************/
  /* Set socket to be nonblocking. All of the sockets for    */
  /* the incoming connections will also be nonblocking since  */
  /* they will inherit that state from the listening socket.   */
  /*************************************************************/
  if (0 > ioctl(this->s_, FIONBIO, (char *)&on)) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category());
  }

  //bind to address
  sp->get<logger>()->trace("UDP", "Starting UDP server on %s", address.to_string().c_str());
  if (0 > ::bind(this->s_, address, address.size())) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category());
  }

  if (0 > ::listen(this->s_, SOMAXCONN)) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category());
  }

  /* Set the socket to non-blocking, this is essential in event
  * based programming with libevent. */

  auto flags = fcntl(this->s_, F_GETFL);
  if (0 > flags) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category());
  }

  flags |= O_NONBLOCK;
  if (0 > fcntl(this->s_, F_SETFL, flags)) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category());
  }

  this->wait_accept_task_ = std::thread(
    [this, sp, ch = std::move(new_connection)]() {
    auto epollfd = epoll_create(1);
    if (0 > epollfd) {
      sp->get<logger>()->error("UDP", "epoll_create failed");
      return;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = this->s_;
    if (0 > epoll_ctl(epollfd, EPOLL_CTL_ADD, this->s_, &ev)) {
      sp->get<logger>()->error("UDP", "epoll_create failed");
      return;
    }

    while (!this->is_shuting_down_ && !sp->get_shutdown_event().is_shuting_down()) {
      auto result = epoll_wait(epollfd, &ev, 1, 1000);
      if (result > 0) {
        sockaddr client_address;
        socklen_t client_address_length = sizeof(client_address);

        auto socket = accept(this->s_, &client_address, &client_address_length);
        if (INVALID_SOCKET != socket) {
          auto s = _tcp_network_socket::from_handle(sp, socket);
          auto r = (*s)->make_socket_non_blocking();
          if (r.has_error()) {
            (void)s->close();
            continue;
          }
          r = (*s)->set_timeouts();
          if (r.has_error()) {
            (void)s->close();
            continue;
          }

          ch(s).then([sp, s](vds::expected<std::shared_ptr<stream_output_async<uint8_t>>> result) {
              if (!result.has_value() && result.value()) {
                auto handler = std::make_shared<_read_socket_task>(sp, s, std::move(result.value()));
                (void)handler->start();
              }
          });
        }
      }
    }
  });
#endif
  return expected<void>();
}

void vds::_tcp_socket_server::stop()
{
}

////////////////
#ifdef _WIN32
vds::_tcp_socket_server::windows_wsa_event::windows_wsa_event()
{
  this->handle_ = WSACreateEvent();
  if (WSA_INVALID_EVENT == this->handle_) {
    auto error = WSAGetLastError();
#if __cpp_exceptions
    throw std::system_error(error, std::system_category(), "WSACreateEvent");
#endif
  }
}

vds::_tcp_socket_server::windows_wsa_event::~windows_wsa_event()
{
  if (WSA_INVALID_EVENT != this->handle_) {
    WSACloseEvent(this->handle_);
  }
}

vds::expected<void> vds::_tcp_socket_server::windows_wsa_event::select(SOCKET s, long lNetworkEvents)
{
  if (SOCKET_ERROR == WSAEventSelect(s, this->handle(), FD_ACCEPT)) {
    auto error = WSAGetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "WSAEventSelect");
  }
  return expected<void>();
}
#endif//
