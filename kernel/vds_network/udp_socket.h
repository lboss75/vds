#ifndef __VDS_NETWORK_UDP_SOCKET_H_
#define __VDS_NETWORK_UDP_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_socket.h"
#include "network_manager.h"

namespace vds {
  
  class udp_socket
  {
  public:
    udp_socket()
    {
      this->s_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        
#ifdef _WIN32
      if (INVALID_SOCKET == this->s_) {
        auto error = WSAGetLastError();
        throw new std::system_error(error, std::system_category(), "create socket");
      }
#else
      if (0 > this->s_) {
        auto error = errno;
        throw new std::system_error(error, std::system_category(), "create socket");
      }
#endif
    }
    
    ~udp_socket()
    {
      this->release();
    }
    
    void release()
    {
#ifdef _WIN32
      if (INVALID_SOCKET != this->s_) {
        closesocket(this->s_);
        this->s_ = INVALID_SOCKET;
      }
#else
      if (0 <= this->s_) {
        shutdown(this->s_, 2);
        this->s_ = -1;
      }
#endif
    }
    
  private:
    network_socket::SOCKET_HANDLE s_;
  };
  
  class udp_server
  {
  public:
    udp_server(
      const service_provider & sp,
      udp_socket & socket,
      const std::string & address,
      int port
    ) : sp_(sp), socket_(socket),
    owner_(sp.get<inetwork_manager>().owner_),
    address_(address), port_(port)
    {
    }
    
    template <typename context_type>
    class handler : public sequence_step<context_type, void(const sockaddr_in &, const void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(const sockaddr_in &, const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const udp_server & args
      ) : base_class(context),
      owner_(args.owner_),
      socket_(args.socket_)
      {
        sockaddr_in addr;
        memset((char *) &addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(args.port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (0 > bind(this->socket_->s_, &addr, sizeof(addr))) {
#ifdef _WIN32
          auto error = WSAGetLastError();
#else
          auto error = errno;
#endif
          throw new std::system_error(error, std::system_category(), "bind socket");
        }
      }
      
      void operator()()
      {
#ifdef _WIN32
#else//_WIN32
        event_set(&this->event_, this->socket_->s_, EV_READ, &handler::callback, this);
        event_add(&this->event_, NULL);
  
        this->owner_->start_libevent_dispatch();
#endif//_WIN32
      }
      
      void processed()
      {
        if(!this->sp_.get_shutdown_event().is_shuting_down()){
          event_set(&this->event_, this->socket_->s_, EV_READ, &handler::callback, this);
          event_add(&this->event_, NULL);
        }
      }
      
    private:
      network_service * owner_;
      udp_socket & socket_;
      struct event event_;
      char buffer_[4096];
      sockaddr_in clientaddr_;
      size_t clientlen_;
      
#ifndef _WIN32
      static void callback(int fd, short event, void *arg)
      {
        auto pthis = reinterpret_cast<handler *>(arg);
        pthis->clientlen_ = sizeof(pthis->clientaddr_);
        
        int len = recvfrom(fd, pthis->buffer_, sizeof(pthis->buffer_), 0,
          (struct sockaddr *)&pthis->clientaddr_, &pthis->clientlen_
        );
        
        if (len < 0) {
          int error = errno;
          throw new std::system_error(error, std::system_category(), "recvfrom");
        }
        
        pthis->next(pthis->clientaddr_, pthis->buffer_, len);
      }      
#endif//_WIN32
    };
  private:
    const service_provider & sp_;
    udp_socket & socket_;
    network_service * owner_;
    std::string address_;
    int port_;    
  };
  
  class udp_client
  {
  public:
    udp_client(
      const service_provider & sp,
      udp_socket & socket,
      const std::string & address,
      int port
    ) : sp_(sp), socket_(socket),
    owner_(sp.get<inetwork_manager>().owner_),
    address_(address), port_(port)
    {
    }
    
  private:
    const service_provider & sp_;
    udp_socket & socket_;
    network_service * owner_;
    std::string address_;
    int port_;    
  };
}

#endif//__VDS_NETWORK_UDP_SOCKET_H_