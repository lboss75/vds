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
    socket_connect(const service_provider & sp)
      : network_service_(sp.get<inetwork_manager>().owner_)
    {
    }

    template <typename context_type>
    class handler : public sequence_step<context_type, void (network_socket &)>
    {
    public:
      handler(
        context_type & context,
        const socket_connect & owner
      )
        : base(context),
        s_(
#ifdef _WIN32
          WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)
#else
          socket(PF_INET, SOCK_STREAM, 0)
#endif//_WIN32
        ), network_service_(owner.network_service_)
      {
      }

      void operator()(
        const std::string & address,
        u_int16_t port
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

        this->network_service_->associate(this->s_.handle());
#else
        // Connect 
        if (0 > ::connect(this->s_.handle(), (struct sockaddr *)&addr, sizeof(addr))) {
          auto error = errno;
          throw new std::system_error(error, std::system_category());
        }
#endif
        this->next(this->s_);
      }

    private:
      network_service * network_service_;
      network_socket s_;
    };
  private:
    network_service * network_service_;
  };
}


#endif//__VDS_NETWORK_SOCKET_CONNECT_H_