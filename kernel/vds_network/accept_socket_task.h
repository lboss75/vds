#ifndef __VDS_NETWORK_ACCEPT_SOCKET_TASK_H_
#define __VDS_NETWORK_ACCEPT_SOCKET_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_manager.h"

namespace vds {
  
  template<
    typename done_method_type,
    typename error_method_type
  >
  class accept_socket_task
  {
  public:
    accept_socket_task(
      done_method_type & done,
      error_method_type & on_error,
      const service_provider & sp,
      const std::string & address,
      int port      
    ) :
      sp_(service_provider::empty()),
      done_method_(done), error_method_(on_error)
#ifndef _WIN32
      , ev_accept_(nullptr)
#endif
    {
#ifdef _WIN32
      this->s_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

      if (INVALID_SOCKET == this->s_) {
          auto error = WSAGetLastError();
          throw std::system_error(error, std::system_category(), "create socket");
      }
#else
      this->s_ = socket(AF_INET, SOCK_STREAM, 0);
      if (this->s_ < 0) {
          auto error = errno;
          throw std::system_error(error, std::system_category());
      }
      /*************************************************************/
      /* Allow socket descriptor to be reuseable                   */
      /*************************************************************/
      int on = 1;
      if (0 > setsockopt(this->s_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
          auto error = errno;
          throw std::system_error(error, std::system_category());
      }

      /*************************************************************/
      /* Set socket to be nonblocking. All of the sockets for    */
      /* the incoming connections will also be nonblocking since  */
      /* they will inherit that state from the listening socket.   */
      /*************************************************************/
      if (0 > ioctl(this->s_, FIONBIO, (char *)&on)) {
          auto error = errno;
          throw std::system_error(error, std::system_category());
      }
#endif
      //bind to address
      sockaddr_in addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port = htons(port);

#ifdef _WIN32
    if (SOCKET_ERROR == ::bind(this->s_, (struct sockaddr *)&addr, sizeof(addr))) {
        auto error = WSAGetLastError();
        throw std::system_error(error, std::system_category(), "bind");
    }

    if (SOCKET_ERROR == ::listen(this->s_, SOMAXCONN)) {
        auto error = WSAGetLastError();
        throw std::system_error(error, std::system_category(), "listen socket");
    }

    this->accept_event_.select(this->s_, FD_ACCEPT);
#else
    if (0 > ::bind(this->s_, (struct sockaddr *)&addr, sizeof(addr))) {
        auto error = errno;
        throw std::system_error(
          error,
          std::system_category());
    }

    if (0 > ::listen(this->s_, SOMAXCONN)) {
        auto error = errno;
        throw std::system_error(
          error,
          std::system_category());
    }

    /* Set the socket to non-blocking, this is essential in event
    * based programming with libevent. */

    auto flags = fcntl(this->s_, F_GETFL);
    if (0 > flags) {
        auto error = errno;
        throw std::system_error(
          error,
          std::system_category());
    }

    flags |= O_NONBLOCK;
    if (0 > fcntl(this->s_, F_SETFL, flags)) {
        auto error = errno;
        throw std::system_error(
          error,
          std::system_category());
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
#endif

      sp.get_shutdown_event().then_shuting_down([this, sp](){
#ifdef _WIN32
        closesocket(this->s_);
#else
        shutdown(this->s_, 2);
#endif
        this->done_method_(sp, nullptr);
      });
    }
    
    ~accept_socket_task()
    {
#ifndef _WIN32
      event_free(this->ev_accept_);
      close(this->s_);
#else
      closesocket(this->s_);
      this->wait_accept_task_.join();
#endif
    }
    
    void schedule(const service_provider & sp)
    {
      this->sp_ = sp;
#ifndef _WIN32
      if(nullptr == this->ev_accept_){
        this->ev_accept_ = event_new(
          static_cast<network_service *>(sp.get<inetwork_manager>())->base_,
          this->s_,
          EV_READ,
          &accept_socket_task::wait_accept,
          this);
      }
      event_add(this->ev_accept_, NULL);
      static_cast<network_service *>(sp.get<inetwork_manager>())->start_libevent_dispatch(sp);
#else
      if (!this->wait_accept_task_.joinable()) {
        this->wait_accept_task_ = std::thread(
          [this, sp]() {
          HANDLE events[2];
          events[0] = this->sp_.get_shutdown_event().windows_handle();
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
                static_cast<network_service *>(sp.get<inetwork_manager>())->associate(socket);
                auto sp = this->sp_.create_scope("Connection from " + network_service::to_string(client_address));
                this->done_method_(sp, &network_socket(socket));
              }
            }
          }
        });
      }
#endif
    }

    
  private:
    service_provider sp_;
    //network_service * network_service_;
    network_socket::SOCKET_HANDLE s_;
    done_method_type & done_method_;
    error_method_type & error_method_;
#ifndef _WIN32
    event * ev_accept_;
    static void wait_accept(int fd, short event, void *arg)
    {
      auto data = reinterpret_cast<accept_socket_task *>(arg);
      
      try {
        sockaddr_in client_addr;
        socklen_t   len = sizeof(client_addr);

        // Accept incoming connection
        int sock = accept(fd, reinterpret_cast<sockaddr *>(&client_addr), &len);
        if (sock < 1) {
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
        auto sp = data->sp_.create_scope("Connection from " + network_service::to_string(client_addr));
        imt_service::async(sp, [sp, data, sock](){
          network_socket s(sock);
          data->done_method_(sp, &s);
        });
      }
      catch(...){
        data->error_method_(data->sp_, std::current_exception());
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
    std::thread wait_accept_task_;
#endif
  };
}

#endif // __VDS_NETWORK_ACCEPT_SOCKET_TASK_H_
