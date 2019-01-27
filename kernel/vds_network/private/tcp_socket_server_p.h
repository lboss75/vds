#ifndef __VDS_NETWORK_SOCKET_SERVER_P_H_
#define __VDS_NETWORK_SOCKET_SERVER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "mt_service.h"
#include "network_types_p.h"
#include "network_service_p.h"
#include "tcp_network_socket_p.h"

namespace vds {
  class _tcp_socket_server
  {
  public:
    _tcp_socket_server()
    : s_(INVALID_SOCKET)
#ifndef _WIN32
      , is_shuting_down_(false)
#endif
    {
    }
    
    ~_tcp_socket_server()
    {
#ifndef _WIN32
      this->is_shuting_down_ = true;
      close(this->s_);
#else
      closesocket(this->s_);
#endif
      if(this->wait_accept_task_.joinable()){
        this->wait_accept_task_.join();
      }
    }

    vds::async_task<vds::expected<void>> start(
      const service_provider * sp,
      const network_address & address,
      const std::function<vds::async_task<vds::expected<void>>(const std::shared_ptr<tcp_network_socket> & s)> & new_connection)
    {
       
#ifdef _WIN32
        this->s_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        if (INVALID_SOCKET == this->s_) {
          const auto error = WSAGetLastError();
          co_return vds::make_unexpected<std::system_error>(error, std::system_category(), "create socket");
        }

        if (SOCKET_ERROR == ::bind(this->s_, address, address.size())) {
          const auto error = WSAGetLastError();
          co_return vds::make_unexpected<std::system_error>(error, std::system_category(), "bind");
        }

        if (SOCKET_ERROR == ::listen(this->s_, SOMAXCONN)) {
          const auto error = WSAGetLastError();
          co_return vds::make_unexpected<std::system_error>(error, std::system_category(), "listen socket");
        }

      auto accept_result = this->accept_event_.select(this->s_, FD_ACCEPT);
      if(accept_result.has_error()) {
        co_return vds::unexpected(std::move(accept_result.error()));
      }

        this->wait_accept_task_ = std::thread(
          [this, sp, new_connection]() {

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
                  (*sp->get<network_service>())->add_connection(new_connection(s));
                }
              }
            }
          }
        });
#else
            this->s_ = socket(AF_INET, SOCK_STREAM, 0);
            if (this->s_ < 0) {
              auto error = errno;
              co_return vds::make_unexpected<std::system_error>(error, std::system_category());
            }
            
            /*************************************************************/
            /* Allow socket descriptor to be reuseable                   */
            /*************************************************************/
            int on = 1;
            if (0 > setsockopt(this->s_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
              auto error = errno;
                co_return vds::make_unexpected<std::system_error>(error, std::system_category());
            }

            /*************************************************************/
            /* Set socket to be nonblocking. All of the sockets for    */
            /* the incoming connections will also be nonblocking since  */
            /* they will inherit that state from the listening socket.   */
            /*************************************************************/
            if (0 > ioctl(this->s_, FIONBIO, (char *)&on)) {
              auto error = errno;
                co_return vds::make_unexpected<std::system_error>(error, std::system_category());
            }
            
            //bind to address
            sp->get<logger>()->trace("UDP", "Starting UDP server on %s", address.to_string().c_str());
            if (0 > ::bind(this->s_, address, address.size())) {
              auto error = errno;
                co_return vds::make_unexpected<std::system_error>(error, std::system_category());
            }

            if (0 > ::listen(this->s_, SOMAXCONN)) {
              auto error = errno;
                co_return vds::make_unexpected<std::system_error>(error, std::system_category());
            }

            /* Set the socket to non-blocking, this is essential in event
            * based programming with libevent. */

            auto flags = fcntl(this->s_, F_GETFL);
            if (0 > flags) {
              auto error = errno;
                co_return vds::make_unexpected<std::system_error>(error, std::system_category());
            }

            flags |= O_NONBLOCK;
            if (0 > fcntl(this->s_, F_SETFL, flags)) {
              auto error = errno;
                co_return vds::make_unexpected<std::system_error>(error, std::system_category());
            }

            this->new_connection_ = new_connection;
            
            this->wait_accept_task_ = std::thread(
              [this, sp](){
                auto epollfd = epoll_create(1);
                if (0 > epollfd) {
                  throw std::runtime_error("epoll_create failed");
                }

                struct epoll_event ev;
                memset(&ev, 0, sizeof(ev));
                ev.events = EPOLLIN;
                ev.data.fd = this->s_;
                if (0 > epoll_ctl(epollfd, EPOLL_CTL_ADD, this->s_, &ev)) {
                  throw std::runtime_error("epoll_create failed");
                }
                
                while(!this->is_shuting_down_ && !sp->get_shutdown_event().is_shuting_down()) {
                  auto result = epoll_wait(epollfd, &ev, 1, 1000);
                  if(result > 0){
                    sockaddr client_address;
                    socklen_t client_address_length = sizeof(client_address);

                    auto socket = accept(this->s_, &client_address, &client_address_length);
                    if (INVALID_SOCKET != socket) {
                      auto s = _tcp_network_socket::from_handle(sp, socket);
                      (void)((*s)->make_socket_non_blocking());
                      (*s)->set_timeouts();
                      (*sp->get<network_service>())->add_connection(this->new_connection_(s));
                    }
                  }
                }
              });
#endif
            co_return expected<void>();
    }
    
    void stop()
    {
    }
    
  private:
    SOCKET_HANDLE s_;
    std::thread wait_accept_task_;
    bool is_shuting_down_;

#ifndef _WIN32
    std::function<vds::async_task<vds::expected<void>>(const std::shared_ptr<tcp_network_socket> & s)> new_connection_;
#else
    class windows_wsa_event
    {
    public:
      windows_wsa_event()
      {
        this->handle_ = WSACreateEvent();
        if (WSA_INVALID_EVENT == this->handle_) {
          auto error = WSAGetLastError();
          throw std::system_error(error, std::system_category(), "WSACreateEvent");
        }
      }

      ~windows_wsa_event()
      {
        if (WSA_INVALID_EVENT != this->handle_) {
          WSACloseEvent(this->handle_);
        }
      }

      expected<void> select(SOCKET s, long lNetworkEvents)
      {
        if (SOCKET_ERROR == WSAEventSelect(s, this->handle(), FD_ACCEPT)) {
          auto error = WSAGetLastError();
          return vds::make_unexpected<std::system_error>(error, std::system_category(), "WSAEventSelect");
        }

        return expected<void>();
      }

      WSAEVENT handle() const {
        return this->handle_;
      }

    private:
      WSAEVENT handle_;
    };

    windows_wsa_event accept_event_;
#endif
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_P_H_
