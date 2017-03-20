#ifndef __VDS_NETWORK_UDP_SOCKET_H_
#define __VDS_NETWORK_UDP_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_socket.h"
#include "network_manager.h"
#include "socket_task.h"

namespace vds {
  class udp_socket
  {
  public:
    udp_socket(const service_provider & sp)
    {
#ifdef _WIN32
      this->s_ = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
      if (INVALID_SOCKET == this->s_) {
        auto error = WSAGetLastError();
        throw new std::system_error(error, std::system_category(), "create socket");
      }

      sp.get<inetwork_manager>().owner_->associate(this->s_);
#else
      this->s_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (0 > this->s_) {
        auto error = errno;
        throw new std::system_error(error, std::system_category(), "create socket");
      }

      /*************************************************************/
      /* Allow socket descriptor to be reuseable                   */
      /*************************************************************/
      int on = 1;
      if (0 > setsockopt(this->s_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
        auto error = errno;
        close(this->s_);
        throw new std::system_error(error, std::system_category(), "Allow socket descriptor to be reuseable");
      }

      /*************************************************************/
      /* Set socket to be nonblocking. All of the sockets for    */
      /* the incoming connections will also be nonblocking since  */
      /* they will inherit that state from the listening socket.   */
      /*************************************************************/
      if (0 > ioctl(this->s_, FIONBIO, (char *)&on)) {
        auto error = errno;
        close(this->s_);
        throw new std::system_error(error, std::system_category(), "Set socket to be nonblocking");
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

    network_socket::SOCKET_HANDLE handle() const
    {
      return this->s_;
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
    class handler
      : public sequence_step<context_type, void(void)>
    {
      using base_class = sequence_step<context_type, void(void)>;
    public:
      handler(
        const context_type & context,
        const udp_server & args
      ) : base_class(context),
        owner_(args.owner_),
        socket_(args.socket_)
      {
        sockaddr_in addr;
        memset((char *)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(args.port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (0 > bind(this->socket_.handle(), (sockaddr *)&addr, sizeof(addr))) {
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
        this->next();
      }


    private:
      network_service * owner_;
      udp_socket & socket_;
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
      int port,
      const void * data,
      size_t len
    ) : sp_(sp), socket_(socket),
      owner_(sp.get<inetwork_manager>().owner_),
      address_(address), port_(port),
      data_(data), len_(len)
    {
    }

    template <typename context_type>
    class handler
      : public sequence_step<context_type, void(void)>,
      public socket_task
    {
      using base_class = sequence_step<context_type, void(void)>;
    public:
      handler(
        const context_type & context,
        const udp_client & args
      ) : base_class(context),
        address_(args.address_),
        port_(args.port_)
      {
        this->s_ = args.socket_.handle();
#ifdef _WIN32
        this->wsa_buf_.len = args.len_;
        this->wsa_buf_.buf = (CHAR *)args.data_;
#endif
      }

      void operator()()
      {
#ifdef _WIN32
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(this->address_.c_str());
        addr.sin_port = htons(this->port_);

        if (NOERROR != WSASendTo(
          this->s_,
          &this->wsa_buf_,
          1,
          NULL,
          0,
          (const sockaddr *)&addr,
          sizeof(addr),
          &this->overlapped_,
          NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw new std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
#endif
      }
#ifdef _WIN32
      void process(DWORD dwBytesTransfered) override
      {
        this->next();
      }
#endif
    private:
      std::string address_;
      int port_;
    };


  private:
    const service_provider & sp_;
    udp_socket & socket_;
    network_service * owner_;
    std::string address_;
    int port_;
    const void * data_;
    size_t len_;
  };

  class udp_send
  {
  public:
    udp_send(
      udp_socket & socket
    )
      : socket_(socket)
    {
    }

    template <typename context_type>
    class handler
      : public sequence_step<context_type, void(void)>,
      public socket_task
    {
      using base_class = sequence_step<context_type, void(void)>;
    public:
      handler(
        const context_type & context,
        const udp_send & args
      ) : base_class(context)
      {
        this->s_ = args.socket_.handle();
      }

      void operator()(const sockaddr_in & to, const void * data, size_t len)
      {
#ifdef _WIN32
        this->wsa_buf_.len = len;
        this->wsa_buf_.buf = (CHAR *)data;

        if (NOERROR != WSASendTo(this->s_, &this->wsa_buf_, 1, NULL, 0, (const sockaddr *)&to, sizeof(to), &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw new std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
#endif
      }
#ifdef _WIN32
      void process(DWORD dwBytesTransfered) override
      {
        if (this->wsa_buf_.len != (size_t)dwBytesTransfered) {
          this->error(new std::runtime_error("Invalid sent UDP data"));
          return;
        }

        this->prev();
      }
#endif
    };

  private:
    udp_socket & socket_;
  };

  class udp_receive
  {
  public:
    udp_receive(
      const service_provider & sp,
      udp_socket & socket
    ) : sp_(sp), socket_(socket)
    {
    }

    template <typename context_type>
    class handler
      : public sequence_step<context_type, void(const sockaddr_in & from, const void * data, size_t len)>
#ifdef _WIN32
      , public socket_task
#endif
    {
      using base_class = sequence_step<context_type, void(const sockaddr_in & from, const void * data, size_t len)>;
    public:
      handler(
        const context_type & context,
        const udp_receive & args
      ) : base_class(context),
        sp_(args.sp_),
        addr_len_(sizeof(addr_))
      {
        this->s_ = args.socket_.handle();
      }

      void operator()()
      {
        this->processed();
      }

      void processed()
      {
        if (!this->sp_.get_shutdown_event().is_shuting_down()) {
#ifdef _WIN32
          this->wsa_buf_.len = sizeof(this->buffer_);
          this->wsa_buf_.buf = this->buffer_;

          DWORD flags = 0;
          DWORD numberOfBytesRecvd;
          if (NOERROR != WSARecvFrom(
            this->s_,
            &this->wsa_buf_,
            1,
            &numberOfBytesRecvd,
            &flags,
            (struct sockaddr*)&this->addr_,
            &this->addr_len_,
            &this->overlapped_, NULL)) {
            auto errorCode = WSAGetLastError();
            if (WSA_IO_PENDING != errorCode) {
              throw new std::system_error(errorCode, std::system_category(), "WSARecvFrom failed");
            }
          }
#else
          event_set(&this->event_, this->s_, EV_READ, &handler::callback, this);
          event_add(&this->event_, NULL);
#endif
        }
      }

#ifdef _WIN32
    void process(DWORD dwBytesTransfered) override
    {
      this->next(this->addr_, this->buffer_, (size_t)dwBytesTransfered);
    }
#endif

  private:
    const service_provider & sp_;
    struct sockaddr_in addr_;
    socklen_t addr_len_;
    char buffer_[4096];

#ifndef _WIN32
    network_socket::SOCKET_HANDLE s_;
    struct event event_;
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<handler *>(arg);
      pthis->addr_len_ = sizeof(pthis->addr_);

      int len = recvfrom(fd, pthis->buffer_, sizeof(pthis->buffer_), 0,
        (struct sockaddr *)&pthis->addr_, &pthis->addr_len_
      );

      if (len < 0) {
        int error = errno;
        throw new std::system_error(error, std::system_category(), "recvfrom");
      }

      pthis->next(pthis->addr_, pthis->buffer_, len);
    }
#endif//_WIN32

    };
private:
  const service_provider & sp_;
  udp_socket & socket_;
  };
  
  template<typename handler_class>
  class _run_udp_server
  {
  public:
    _run_udp_server(
      const service_provider & sp,
      udp_socket & s,
      handler_class & owner)
    : sp_(sp), s_(s), owner_(owner)
    {
    }
    
    template <typename context_type>
    class handler : public sequence_step<context_type, void(void)>
    {
      using base_class = sequence_step<context_type, void(void)>;
    public:
      handler(
        const context_type & context,
        const _run_udp_server & args)
      : base_class(context),
        sp_(args.sp_),
        s_(args.s_),
        owner_(args.owner_),
        multi_handler_(2, [this](std::list<std::exception *> & errors) { this->owner_.socket_closed(errors); })
      {
      }
      
      void operator()()
      {
        sequence(
          udp_receive(this->sp_, this->s_),
          create_step<write_handler>::with<handler_class &>(this->owner_)
        )(          
          this->multi_handler_.on_done(),
          this->multi_handler_.on_error()
        );
        
        sequence(
          create_step<read_handler>::with<handler_class &>(this->owner_),
          udp_send(this->s_)
        )(
          this->multi_handler_.on_done(),
          this->multi_handler_.on_error()
        );
      }
      
    private:
      service_provider sp_;
      udp_socket & s_;
      handler_class & owner_;
      multi_handler multi_handler_;
    };
    
    template <typename context_type>
    class write_handler : public sequence_step<context_type, void(void)>
    {
      using base_class = sequence_step<context_type, void(void)>;
    public:
      write_handler(
        const context_type & context,
        handler_class & owner)
      : base_class(context),
        owner_(owner) {

      }

      void operator()(const sockaddr_in & from, const void * data, size_t len)
      {
        this->owner_.input_message(from, data, len);
      }

    private:
      handler_class & owner_;
    };

    template <typename context_type>
    class read_handler : public sequence_step<context_type, void(const sockaddr_in & from, const void * data, size_t len)>
    {
      using base_class = sequence_step<context_type, void(const sockaddr_in & from, const void * data, size_t len)>;
    public:
      read_handler(
        const context_type & context,
        handler_class & owner
      ) : base_class(context),
      owner_(owner) {
      }

      void operator()()
      {
        this->owner_.get_message(this->next);
      }

    private:
      handler_class & owner_;
    };

  private:
    service_provider sp_;
    udp_socket & s_;
    handler_class & owner_;
  };

  
  template<typename handler_class>
  inline void run_udp_server(
    const service_provider & sp,
    udp_socket & s,
    const std::string & address,
    size_t port,
    handler_class & handler)
  {
    sequence(
      udp_server(sp, s, address, port),
      _run_udp_server<handler_class>(sp, s, handler)
    )
    (
     [&handler]() { handler.udp_server_done(); },
     [&handler](std::exception * ex) { handler.udp_server_error(ex);}
    );
  }

  template<typename handler_class>
  inline void run_udp_server(
    const service_provider & sp,
    udp_socket & s,
    const std::string & address,
    size_t port,
    const std::function<void(void)> & done_handler,
    const std::function<void(std::exception *)> & error_handler)
  {
    sequence(
      udp_server(sp, s, address, port),
      _run_udp_server<handler_class>()
    )
    (
     done_handler,
     error_handler
    );
  }
  
  template<typename handler_class, typename done_handler_type, typename error_handler_type>
  inline void run_udp_server(
    const service_provider & sp,
    udp_socket & s,
    const std::string & address,
    size_t port,
    done_handler_type & done_handler,
    error_handler_type & error_handler)
  {
    sequence(
      udp_server(sp, s, address, port),
      _run_udp_server<handler_class>()
    )
    (
     done_handler,
     error_handler
    );
  }
  
  
}

#endif//__VDS_NETWORK_UDP_SOCKET_H_