/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "tcp_network_socket.h"
#include "tcp_network_socket_p.h"
#include "read_socket_task_p.h"
#include "write_socket_task_p.h"

vds::tcp_network_socket::tcp_network_socket()
: impl_(new _tcp_network_socket())
{
}

vds::tcp_network_socket::tcp_network_socket(const std::shared_ptr< vds::_tcp_network_socket >& impl)
: impl_(impl)
{

}


vds::tcp_network_socket::~tcp_network_socket()
{  
}

vds::async_task< const vds::tcp_network_socket &> vds::tcp_network_socket::connect(
  const vds::service_provider& sp,
  const std::string & server,
  const uint16_t port)
{
  return create_async_task([server, port](
    const std::function<void(const service_provider & sp, const tcp_network_socket &)> & done,
    const error_handler & on_error,
    const service_provider & sp){
      auto s = std::make_shared<_tcp_network_socket>(
#ifdef _WIN32
        WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)
#else
        socket(PF_INET, SOCK_STREAM, 0)
#endif//_WIN32
        );

      // Connexion setting for local connexion 
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr(server.c_str());
      addr.sin_port = htons(port);

#ifdef _WIN32
      // Connect 
      if (SOCKET_ERROR == ::connect(s->handle(), (struct sockaddr *)&addr, sizeof(addr))) {
        // As we are in non-blocking mode we'll always have the error 
        // WSAEWOULDBLOCK whichis actually not one 
        auto error = WSAGetLastError();
        if (WSAEWOULDBLOCK != error) {
          on_error(sp, std::make_unique<std::system_error>(error, std::system_category(), "connect"));
          return;
        }
      }

      static_cast<_network_service *>(sp.get<inetwork_service>())->associate(s->handle());
#else
      // Connect 
      if (0 > ::connect(s->handle(), (struct sockaddr *)&addr, sizeof(addr))) {
        auto error = errno;
        on_error(sp, std::make_shared<std::system_error>(error, std::generic_category()));
      }
#endif
      tcp_network_socket sc(s);
      sc->start(sp);
      done(sp, sc);
    });
}

std::shared_ptr<vds::continuous_stream<uint8_t>> vds::tcp_network_socket::incoming() const
{
  return this->impl_->incoming();
}

std::shared_ptr<vds::continuous_stream<uint8_t>> vds::tcp_network_socket::outgoing() const
{
  return this->impl_->outgoing();
}

void vds::tcp_network_socket::close()
{
  this->impl_->close();
}

