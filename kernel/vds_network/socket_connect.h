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

    template <typename context_type>
    class handler : public dataflow_step<context_type, void (network_socket &)>
    {
    public:
      handler(
        const context_type & context,
        const socket_connect & owner
      )
        : dataflow_step<context_type, void (network_socket &)>(context),
        s_(
#ifdef _WIN32
          WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)
#else
          socket(PF_INET, SOCK_STREAM, 0)
#endif//_WIN32
        )
      {
      }

      void operator()(
        const service_provider & sp,
        const std::string & address,
        uint16_t port
      )
      {
        // Connexion setting for local connexion 
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(address.c_str());
        addr.sin_port = htons(port);

#ifdef _WIN32
        // Connect 
        if (SOCKET_ERROR == ::connect(this->s_.handle(), (struct sockaddr *)&addr, sizeof(addr))) {
          // As we are in non-blocking mode we'll always have the error 
          // WSAEWOULDBLOCK whichis actually not one 
          auto error = WSAGetLastError();
          if (WSAEWOULDBLOCK != error) {
            throw new std::system_error(error, std::system_category(), "connect");
          }
        }

        ((network_service *)&sp.get<inetwork_manager>())->associate(this->s_.handle());
#else
        // Connect 
        if (0 > ::connect(this->s_.handle(), (struct sockaddr *)&addr, sizeof(addr))) {
          auto error = errno;
          throw std::system_error(error, std::generic_category());
        }
#endif
        this->next(sp, this->s_);
      }
      
      void processed(const service_provider & sp)
      {
      }

    private:
      network_socket s_;
    };
  private:
  };
}


#endif//__VDS_NETWORK_SOCKET_CONNECT_H_