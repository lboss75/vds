#ifndef __VDS_NETWORK_SOCKET_SERVER_P_H_
#define __VDS_NETWORK_SOCKET_SERVER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"
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
      , ev_accept_(nullptr), sp_(service_provider::empty())
#endif
    {
    }
    
    ~_tcp_socket_server()
    {
#ifndef _WIN32
      if(nullptr != this->ev_accept_){
        event_free(this->ev_accept_);
      }
      close(this->s_);
#else
      closesocket(this->s_);
      if(this->wait_accept_task_.joinable()){
        this->wait_accept_task_.join();
      }
#endif
    }

    async_task<> start(
      const service_provider & sp,
      const std::string & address,
      int port,
      const std::function<void(const service_provider & sp, const tcp_network_socket & s)> & new_connection)
    {
      imt_service::async_enabled_check(sp);
      return create_async_task([this, address, port, new_connection](
        const std::function<void (const service_provider & sp)> & done,
        const error_handler & on_error,
        const service_provider & sp) {
        
#ifdef _WIN32
        this->s_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        if (INVALID_SOCKET == this->s_) {
          auto error = WSAGetLastError();
          throw std::system_error(error, std::system_category(), "create socket");
        }

        //bind to address
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (SOCKET_ERROR == ::bind(this->s_, (struct sockaddr *)&addr, sizeof(addr))) {
          auto error = WSAGetLastError();
          throw std::system_error(error, std::system_category(), "bind");
        }

        if (SOCKET_ERROR == ::listen(this->s_, SOMAXCONN)) {
          auto error = WSAGetLastError();
          throw std::system_error(error, std::system_category(), "listen socket");
        }

        this->accept_event_.select(this->s_, FD_ACCEPT);

        sp.get_shutdown_event().then_shuting_down([this, sp]() {
          closesocket(this->s_);
        });

        this->wait_accept_task_ = std::thread(
          [this, sp, new_connection]() {

            HANDLE events[2];
            events[0] = sp.get_shutdown_event().windows_handle();
            events[1] = this->accept_event_.handle();

            for(;;){
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
                  static_cast<_network_service *>(sp.get<inetwork_service>())->associate(socket);
                  auto scope = sp.create_scope(("Connection from " + network_service::to_string(client_address)).c_str());
                  new_connection(scope, _tcp_network_socket::from_handle(socket));
                }
              }
            }
          });
          done(sp);
#else
            this->s_ = socket(AF_INET, SOCK_STREAM, 0);
            if (this->s_ < 0) {
              auto error = errno;
              on_error(sp, std::make_shared<std::system_error>(error, std::system_category()));
              return;
            }
            
            /*************************************************************/
            /* Allow socket descriptor to be reuseable                   */
            /*************************************************************/
            int on = 1;
            if (0 > setsockopt(this->s_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
              auto error = errno;
              on_error(sp, std::make_shared<std::system_error>(error, std::system_category()));
              return;
            }

            /*************************************************************/
            /* Set socket to be nonblocking. All of the sockets for    */
            /* the incoming connections will also be nonblocking since  */
            /* they will inherit that state from the listening socket.   */
            /*************************************************************/
            if (0 > ioctl(this->s_, FIONBIO, (char *)&on)) {
              auto error = errno;
              on_error(sp, std::make_shared<std::system_error>(error, std::system_category()));
              return;
            }
            
            //bind to address
            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            addr.sin_port = htons(port);
            if (0 > ::bind(this->s_, (struct sockaddr *)&addr, sizeof(addr))) {
              auto error = errno;
              on_error(sp, std::make_shared<std::system_error>(error, std::system_category()));
              return;
            }

            if (0 > ::listen(this->s_, SOMAXCONN)) {
              auto error = errno;
              on_error(sp, std::make_shared<std::system_error>(error, std::system_category()));
              return;
            }

            /* Set the socket to non-blocking, this is essential in event
            * based programming with libevent. */

            auto flags = fcntl(this->s_, F_GETFL);
            if (0 > flags) {
              auto error = errno;
              on_error(sp, std::make_shared<std::system_error>(error, std::system_category()));
              return;
            }

            flags |= O_NONBLOCK;
            if (0 > fcntl(this->s_, F_SETFL, flags)) {
              auto error = errno;
              on_error(sp, std::make_shared<std::system_error>(error, std::system_category()));
              return;
            }

            /* We now have a listening socket, we create a read event to
              * be notified when a client connects. * /
            event_set(
              &this->ev_accept_,
              this->s_,
              EV_READ,
              &accept_socket_task::wait_accept,
              this);
            event_add(&this->ev_accept_, NULL);
            this->network_service_->start_libevent_dispatch();*/

            sp.get_shutdown_event().then_shuting_down([this, sp, done](){
              shutdown(this->s_, 2);
            });
            this->sp_ = sp;
            this->done_ = done;
            this->on_error_ = on_error;
            this->new_connection_ = new_connection;
            this->ev_accept_ = event_new(
              static_cast<_network_service *>(sp.get<inetwork_service>())->base_,
              this->s_,
              EV_READ | EV_PERSIST,
              &_tcp_socket_server::wait_accept,
              this);
            event_add(this->ev_accept_, NULL);
            static_cast<_network_service *>(sp.get<inetwork_service>())->start_libevent_dispatch(sp);
            done(sp);
#endif
      });
    }
    
    void stop(const service_provider & sp)
    {
    }
    
  private:
    SOCKET_HANDLE s_;
    std::thread wait_accept_task_;
#ifndef _WIN32
    std::function<void(const service_provider & sp, const tcp_network_socket & s)> new_connection_;
    event * ev_accept_;
    service_provider sp_;
    std::function<void (const service_provider & sp)> done_;
    error_handler on_error_;
    static void wait_accept(int fd, short event, void *arg)
    {
      auto data = reinterpret_cast<_tcp_socket_server *>(arg);
      
      try {
        sockaddr_in client_addr;
        socklen_t   len = sizeof(client_addr);

        // Accept incoming connection
        int sock = accept(fd, reinterpret_cast<sockaddr *>(&client_addr), &len);
        if (sock < 1) {
          data->done_(data->sp_);
          return;
        }
        
        /* Set the socket to non-blocking, this is essential in event
        * based programming with libevent. */

        auto flags = fcntl(sock, F_GETFL);
        if (0 > flags) {
          auto error = errno;
          throw std::system_error(
            error,
            std::system_category());
        }

        flags |= O_NONBLOCK;
        if (0 > fcntl(sock, F_SETFL, flags)) {
          auto error = errno;
          throw std::system_error(
            error,
            std::system_category(),
            "fcntl");
        }
        
        /*************************************************************/
        /* Set socket to be nonblocking. All of the sockets for    */
        /* the incoming connections will also be nonblocking since  */
        /* they will inherit that state from the listening socket.   */
        /*************************************************************/
        //int on = 1;
        //if (0 > ioctl(sock, FIONBIO, (char *)&on)) {
        //    auto error = errno;
        //    throw c_exception("Set socket to be nonblocking", error);
        //}
        auto sp = data->sp_.create_scope(("Connection from " + network_service::to_string(client_addr)).c_str());
        imt_service::async(sp, [sp, data, sock](){
          data->new_connection_(sp, _tcp_network_socket::from_handle(sock));
        });
      }
      catch (const std::exception & ex) {
        data->on_error_(data->sp_, std::make_shared<std::exception>(ex));
      }
      catch (...) {
        data->on_error_(data->sp_, std::make_shared<std::runtime_error>("Unexpected error"));
      }
    }

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

      void select(SOCKET s, long lNetworkEvents)
      {
        if (SOCKET_ERROR == WSAEventSelect(s, this->handle(), FD_ACCEPT)) {
          auto error = WSAGetLastError();
          throw std::system_error(error, std::system_category(), "WSAEventSelect");
        }
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
