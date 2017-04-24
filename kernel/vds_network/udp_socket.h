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
      : public dataflow_step<context_type, void(void)>
    {
      using base_class = dataflow_step<context_type, void(void)>;
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

  class udp_send
  {
  public:
    udp_send(
      const service_provider & sp,
      udp_socket & socket)
      : sp_(sp), socket_(socket)
    {
    }

    template <typename context_type>
    class handler
      : public dataflow_step<context_type, void(void)>,
      public socket_task
    {
      using base_class = dataflow_step<context_type, void(void)>;
    public:
      handler(
        const context_type & context,
        const udp_send & args
      ) : base_class(context), sp_(args.sp_),
        log_(args.sp_, "UDP Send")
#ifndef _WIN32
      , network_service_(args.sp_.get<inetwork_manager>().owner())
#endif
      {
        this->s_ = args.socket_.handle();
      }

      void operator()(const sockaddr_in * to, const void * data, size_t len)
      {
        if(0 == len){
          this->next();
          return;
        }
        
#ifdef _WIN32
        this->wsa_buf_.len = len;
        this->wsa_buf_.buf = (CHAR *)data;

        if (NOERROR != WSASendTo(this->s_, &this->wsa_buf_, 1, NULL, 0, (const sockaddr *)to, sizeof(*to), &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw new std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
#else
        this->to_ = to;
        this->data_ = (const unsigned char *)data;
        this->data_size_ = len;
        
        if(nullptr == this->event_) {
          this->event_ = event_new(
            this->network_service_->base_,
            this->s_,
            EV_WRITE,
            &callback,
            this);
        }
        // Schedule client event
        event_add(this->event_, NULL);
        
        this->network_service_->start_libevent_dispatch(this->sp_);
#endif
      }
#ifdef _WIN32
      void process(DWORD dwBytesTransfered) override
      {
        try {
          if (this->wsa_buf_.len != (size_t)dwBytesTransfered) {
            throw std::runtime_error("Invalid sent UDP data");
          }

          this->prev();
        }
        catch (...) {
          this->error(std::current_exception());
        }
      }
#endif

    private:
      service_provider sp_;
      logger log_;
      network_socket::SOCKET_HANDLE s_;
    
#ifndef _WIN32
      const sockaddr_in * to_;
      const unsigned char * data_;
      size_t data_size_;
      network_service * network_service_;
      
      static void callback(int fd, short event, void *arg)
      {
        auto pthis = reinterpret_cast<handler *>(arg);
        try {
          pthis->log_.trace("Send %d bytes to %s", pthis->data_size_, network_service::to_string(*pthis->to_).c_str());
          
          int len = sendto(fd, pthis->data_, pthis->data_size_, 0, (sockaddr *)pthis->to_, sizeof(*pthis->to_));
          if (len < 0) {
            int error = errno;
            throw std::system_error(
                error,
                std::generic_category(),
                "Send to " + network_service::to_string(*pthis->to_));
          }
          
          pthis->data_ += len;
          pthis->data_size_ -= len;
          if (0 < pthis->data_size_) {
            //event_set(&pthis->event_, pthis->s_, EV_WRITE, &write_socket_task::callback, pthis);
            event_add(pthis->event_, NULL);
          }
          else {
            imt_service::async(pthis->sp_, 
              [pthis](){ pthis->prev(); });
          }
        }
        catch(...){
          pthis->error(std::current_exception());
        }
      }
#endif//_WIN32
    };
  private:
    service_provider sp_;
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
      : public dataflow_step<context_type, void(const sockaddr_in * from, const void * data, size_t len)>
#ifdef _WIN32
      , public socket_task
#endif
    {
      using base_class = dataflow_step<context_type, void(const sockaddr_in * from, const void * data, size_t len)>;
    public:
      handler(
        const context_type & context,
        const udp_receive & args
      ) : base_class(context),
        sp_(args.sp_),
        log_(args.sp_, "UDP Receive"),
        buffer_(this)
#ifndef _WIN32
      , event_(nullptr)
      , network_service_(args.sp_.get<inetwork_manager>().owner())
#endif
      {
        this->s_ = args.socket_.handle();
      }

      void operator()()
      {
        this->buffer_.start();
      }

      void processed()
      {
        if (!this->sp_.get_shutdown_event().is_shuting_down()) {
          this->buffer_.dequeue();
        }
      }

      void data_require(uint8_t * data, uint32_t data_size){
        if (!this->sp_.get_shutdown_event().is_shuting_down()) {
#ifdef _WIN32
          auto addr = (struct sockaddr *)data;
          data += sizeof(sockaddr_in);
          auto addr_len = (INT *)data;
          *addr_len = sizeof(sockaddr_in);
          data += sizeof(INT);

          this->wsa_buf_.len = data_size - sizeof(sockaddr_in) - sizeof(INT);
          this->wsa_buf_.buf = (CHAR *)data;

          DWORD flags = 0;
          DWORD numberOfBytesRecvd;
          if (NOERROR != WSARecvFrom(
            this->s_,
            &this->wsa_buf_,
            1,
            &numberOfBytesRecvd,
            &flags,
            addr,
            addr_len,
            &this->overlapped_, NULL)) {
            auto errorCode = WSAGetLastError();
            if (WSA_IO_PENDING != errorCode) {
              throw new std::system_error(errorCode, std::system_category(), "WSARecvFrom failed");
            }
          }
#else
          if(nullptr == this->event_) {
            this->event_ = event_new(
              this->network_service_->base_,
              this->s_,
              EV_READ,
              &callback,
              this);
          }
          // Schedule client event
          event_add(this->event_, NULL);
          
          this->network_service_->start_libevent_dispatch(this->sp_);
#endif
        }
      }

#ifdef _WIN32
    void process(DWORD dwBytesTransfered) override
    {
      this->buffer_.queue((uint32_t)(dwBytesTransfered + sizeof(sockaddr_in) + sizeof(INT)));
    }
#endif

    void push_data(const uint8_t * data, uint32_t data_size)
    {
      auto addr = (struct sockaddr_in *)data;
      data += sizeof(sockaddr_in);
      auto addr_len = (INT *)data;
      data += sizeof(INT);

      this->next(addr, data, data_size - sizeof(sockaddr_in) - sizeof(INT));
    }

  private:
    const service_provider & sp_;
    logger log_;

    circular_buffer<handler, 1 * 1024 * 1024, 8 * 1024> buffer_;

#ifndef _WIN32
    network_socket::SOCKET_HANDLE s_;
    struct event * event_;
    network_service * network_service_;
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<handler *>(arg);
      pthis->addr_len_ = sizeof(pthis->addr_);

      int len = recvfrom(fd, pthis->buffer_, sizeof(pthis->buffer_), 0,
        (struct sockaddr *)&pthis->addr_, &pthis->addr_len_
      );
      
      pthis->log_.trace("Receive %d bytes from %s", len, network_service::to_string(pthis->addr_).c_str());

      if (len < 0) {
        int error = errno;
        throw new std::system_error(error, std::system_category(), "recvfrom");
      }

      pthis->next(&pthis->addr_, pthis->buffer_, len);
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
    class handler : public dataflow_step<context_type, void(void)>
    {
      using base_class = dataflow_step<context_type, void(void)>;
    public:
      handler(
        const context_type & context,
        const _run_udp_server & args)
      : base_class(context),
        sp_(args.sp_),
        s_(args.s_),
        owner_(args.owner_),
        multi_handler_(2, [this](std::list<std::exception_ptr> & errors) { this->owner_.socket_closed(errors); })
      {
      }
      
      void operator()()
      {
        dataflow(
          udp_receive(this->sp_, this->s_),
          create_step<write_handler>::with(this->owner_)
        )(          
          this->multi_handler_.on_done(),
          this->multi_handler_.on_error()
        );
        
        dataflow(
          create_step<read_handler>::with(this->owner_),
          udp_send(this->sp_, this->s_)
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
    class write_handler : public dataflow_step<context_type, void(void)>
    {
      using base_class = dataflow_step<context_type, void(void)>;
    public:
      write_handler(
        const context_type & context,
        handler_class & owner)
      : base_class(context),
        owner_(owner) {

      }

      void operator()(const sockaddr_in * from, const void * data, size_t len)
      {
        if (0 == len) {
          this->next();
        }
        else {
          this->owner_
            .input_message(from, data, len)
            .wait(
              [this]() { this->prev(); },
              [this](std::exception_ptr ex) { this->error(ex); });
        }
      }

    private:
      handler_class & owner_;
    };

    template <typename context_type>
    class read_handler : public dataflow_step<context_type, void(const sockaddr_in * from, const void * data, size_t len)>
    {
      using base_class = dataflow_step<context_type, void(const sockaddr_in * from, const void * data, size_t len)>;
    public:
      read_handler(
        const context_type & context,
        handler_class & owner
      ) : base_class(context),
        owner_(owner),
        handler_(*this)
      {
      }

      void operator()()
      {
        this->owner_.get_message(this->handler_);
      }

    private:
      handler_class & owner_;
      
      class handler
      {
      public:
        handler(read_handler & owner)
        : owner_(owner)
        {
        }
        
        void operator()(const std::string & address, uint16_t port, const const_data_buffer & data)
        {
          this->data_ = data;
          this->from_.sin_family = AF_INET;
          this->from_.sin_addr.s_addr = inet_addr(address.c_str());
          this->from_.sin_port = htons(port);

          this->owner_.next(&this->from_, this->data_.data(), this->data_.size());
        }
        
      private:
        read_handler & owner_;
        sockaddr_in from_;
        const_data_buffer data_;
      };
      handler handler_;
    };

  private:
    service_provider sp_;
    udp_socket & s_;
    handler_class & owner_;
  };

  
  template<typename handler_class>
  inline async_task<> run_udp_server(
    const service_provider & sp,
    udp_socket & s,
    const std::string & address,
    size_t port,
    handler_class & handler)
  {
    return create_async_task(
      [sp, &s, address, port, &handler] (const std::function<void(void)> & done, const error_handler & on_error){
        dataflow(
          udp_server(sp, s, address, port),
          _run_udp_server<handler_class>(sp, s, handler)
        )
        (
          done,
          on_error
        );
      });
  }

  template<typename handler_class>
  inline void run_udp_server(
    const service_provider & sp,
    udp_socket & s,
    const std::string & address,
    size_t port,
    const std::function<void(void)> & done_handler,
    const std::function<void(std::exception_ptr)> & error_handler)
  {
    dataflow(
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
    dataflow(
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