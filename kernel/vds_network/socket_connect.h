#ifndef __VDS_NETWORK_SOCKET_CONNECT_H_
#define __VDS_NETWORK_SOCKET_CONNECT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_socket.h"

namespace vds {
  class socket_connect
  {
  public:
    socket_connect()
    {
    }

    vds::async_task<network_socket &> connect(const std::string & address, uint16_t port)
    {
      return create_vds::async_task([address, port](const std::function<void(const service_provider & sp, network_socket &)> & done, const error_handler & on_error, const service_provider & sp){
        network_socket s(
#ifdef _WIN32
          WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)
#else
          socket(PF_INET, SOCK_STREAM, 0)
#endif//_WIN32
        );

        // Connexion setting for local connexion 
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(address.c_str());
        addr.sin_port = htons(port);

#ifdef _WIN32
        // Connect 
        if (SOCKET_ERROR == ::connect(s.handle(), (struct sockaddr *)&addr, sizeof(addr))) {
          // As we are in non-blocking mode we'll always have the error 
          // WSAEWOULDBLOCK whichis actually not one 
          auto error = WSAGetLastError();
          if (WSAEWOULDBLOCK != error) {
            on_error(sp, std::make_exception_ptr(std::system_error(error, std::system_category(), "connect")));
            return;
          }
        }

        ((network_service *)sp.get<inetwork_manager>())->associate(s.handle());
#else
        // Connect 
        if (0 > ::connect(s.handle(), (struct sockaddr *)&addr, sizeof(addr))) {
          auto error = errno;
          on_error(sp, std::make_exception_ptr(std::system_error(error, std::generic_category())));
        }
#endif
        done(sp, s);
      });
    }
  };
}


#endif//__VDS_NETWORK_SOCKET_CONNECT_H_