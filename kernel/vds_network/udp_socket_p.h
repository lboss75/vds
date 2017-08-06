#ifndef __VDS_NETWORK_UDP_SOCKET_P_H_
#define __VDS_NETWORK_UDP_SOCKET_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_types_p.h"
#include "service_provider.h"
#include "network_service_p.h"
#include "udp_socket.h"
#include "socket_task_p.h"
#include "const_data_buffer.h"

namespace vds {

  class _udp_datagram
  {
  public:
    _udp_datagram(
      const std::string & server,
      uint16_t port,
      const void * data,
      size_t data_size)
      : data_(data, data_size)
    {
      memset((char *)&this->addr_, 0, sizeof(this->addr_));
      this->addr_.sin_family = AF_INET;
      this->addr_.sin_port = htons(port);
      this->addr_.sin_addr.s_addr = inet_addr(server.c_str());
    }

    _udp_datagram(
      const std::string & server,
      uint16_t port,
      const const_data_buffer & data)
      : data_(data)
    {
      memset((char *)&this->addr_, 0, sizeof(this->addr_));
      this->addr_.sin_family = AF_INET;
      this->addr_.sin_port = htons(port);
      this->addr_.sin_addr.s_addr = inet_addr(server.c_str());
    }

    _udp_datagram(
      const sockaddr_in & addr,
      const void * data,
      size_t data_size)
      : addr_(addr), data_(data, data_size)
    {
    }

    _udp_datagram(
      const sockaddr_in & addr,
      const const_data_buffer & data)
      : addr_(addr), data_(data)
    {
    }

    sockaddr_in * addr() { return &this->addr_; }
    
    std::string server() const { return network_service::get_ip_address_string(this->addr_); }
    uint16_t port() const { return ntohs(this->addr_.sin_port); }

    const void * data() const { return this->data_.data(); }
    size_t data_size() const { return this->data_.size(); }

    static udp_datagram create(const sockaddr_in & addr, const void * data, size_t data_size)
    {
      return udp_datagram(new _udp_datagram(addr, data, data_size));
    }

    static udp_datagram create(const sockaddr_in & addr, const const_data_buffer & data)
    {
      return udp_datagram(new _udp_datagram(addr, data));
    }

  private:
    sockaddr_in addr_;
    const_data_buffer data_;
  };


  class _udp_socket
  {
  public:
    _udp_socket()
      : s_(INVALID_SOCKET),
      receive_(*this), send_(*this)
    {
    }

    ~_udp_socket()
    {
      this->close();
    }

    void create(const service_provider & sp)
    {
#ifdef _WIN32
      this->s_ = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
      if (INVALID_SOCKET == this->s_) {
        auto error = WSAGetLastError();
        throw std::system_error(error, std::system_category(), "create socket");
      }

      static_cast<_network_service *>(sp.get<inetwork_service>())->associate(this->s_);
#else
      this->s_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (0 > this->s_) {
        auto error = errno;
        throw std::system_error(error, std::system_category(), "create socket");
      }

      /*************************************************************/
      /* Allow socket descriptor to be reuseable                   */
      /*************************************************************/
      int on = 1;
      if (0 > setsockopt(this->s_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
        auto error = errno;
        ::close(this->s_);
        throw std::system_error(error, std::system_category(), "Allow socket descriptor to be reuseable");
      }

      /*************************************************************/
      /* Set socket to be nonblocking. All of the sockets for    */
      /* the incoming connections will also be nonblocking since  */
      /* they will inherit that state from the listening socket.   */
      /*************************************************************/
      if (0 > ioctl(this->s_, FIONBIO, (char *)&on)) {
        auto error = errno;
        ::close(this->s_);
        throw std::system_error(error, std::system_category(), "Set socket to be nonblocking");
      }
#endif
    }

    void close()
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

    SOCKET_HANDLE handle() const
    {
      return this->s_;
    }

    std::shared_ptr<continuous_stream<udp_datagram>> incoming()
    {
      return this->receive_.stream();
    }

    std::shared_ptr<async_stream<udp_datagram>> outgoing()
    {
      return this->send_.stream();
    }

    void start(const vds::service_provider & sp)
    {
      this->receive_.start(sp);
      this->send_.start(sp);
    }

  private:
    SOCKET_HANDLE s_;

    class _udp_receive
#ifdef _WIN32
      : public _socket_task
#endif
    {
    public:
      _udp_receive(_udp_socket & owner)
        : owner_(owner),
        sp_(service_provider::empty()),
        target_(new continuous_stream<udp_datagram>())
#ifndef _WIN32
        , event_(nullptr)
#endif
      {
      }

      void start(const service_provider & sp)
      {
        this->read_async(sp);
      }

      std::shared_ptr<continuous_stream<udp_datagram>> stream()
      {
        return this->target_;
      }

    private:
      _udp_socket & owner_;
      service_provider sp_;
      std::shared_ptr<continuous_stream<udp_datagram>> target_;
      sockaddr_in addr_;
      socklen_t addr_len_;
      uint8_t buffer_[10 * 1024 * 1024];

      void read_async(const vds::service_provider & sp)
      {
        this->sp_ = sp;
        this->addr_len_ = sizeof(sockaddr_in);
        
#ifdef _WIN32

        this->wsa_buf_.len = sizeof(this->buffer_);
        this->wsa_buf_.buf = (CHAR *)this->buffer_;

        DWORD flags = 0;
        DWORD numberOfBytesRecvd;
        if (NOERROR != WSARecvFrom(
          this->owner_.handle(),
          &this->wsa_buf_,
          1,
          &numberOfBytesRecvd,
          &flags,
          (struct sockaddr *)&this->addr_,
          &this->addr_len_,
          &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw std::system_error(errorCode, std::system_category(), "WSARecvFrom failed");
          }
        }
#else
        if (nullptr == this->event_) {
          this->event_ = event_new(
            static_cast<_network_service *>(sp.get<inetwork_service>())->base_,
            this->owner_.handle(),
            EV_READ,
            &callback,
            this);
        }
        // Schedule client event
        event_add(this->event_, NULL);

        static_cast<_network_service *>(sp.get<inetwork_service>())->start_libevent_dispatch(sp);
#endif
      }

#ifdef _WIN32
      void process(DWORD dwBytesTransfered) override
      {
        this->push_data(dwBytesTransfered);
      }

      void error(DWORD error_code) override
      {
      }

#else
      struct event * event_;
      not_mutex event_mutex_;


      static void callback(int fd, short event, void *arg)
      {
        auto pthis = reinterpret_cast<_udp_receive *>(arg);
        std::unique_lock<not_mutex> lock(pthis->event_mutex_);

        int len = recvfrom(fd, pthis->buffer_, sizeof(pthis->buffer_), 0,
          (struct sockaddr *)&pthis->addr_, &pthis->addr_len_);

        if (len < 0) {
          int error = errno;
          if(EAGAIN == error){
            // Schedule client event
            event_add(pthis->event_, NULL);
            return;
          }
          throw std::system_error(error, std::system_category(), "recvfrom");
        }
        
        pthis->sp_.get<logger>()->debug(pthis->sp_, "UDP got %d bytes from %s", len, network_service::to_string(pthis->addr_).c_str());

        pthis->push_data(len);
      }
#endif//_WIN32
      void push_data(size_t readed)
      {
        this->target_->write_value_async(this->sp_, _udp_datagram::create(this->addr_, this->buffer_, readed))
          .wait(
            [this](const service_provider & sp) {
              this->read_async(sp);
            },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            },
            this->sp_);
      }
    };

    class _udp_send
#ifdef _WIN32
      : public _socket_task
#endif
    {
    public:
      _udp_send(
        _udp_socket & owner)
        : owner_(owner),
        sp_(service_provider::empty()),
        source_(new async_stream<udp_datagram>())
#ifndef _WIN32
        , event_(nullptr)
#endif
      {
      }


      not_mutex start_mutex_;
      
      void start(const service_provider & sp)
      {
        //std::cout << this << "->_udp_send::start " << syscall(SYS_gettid) << ": lock\n";

        this->start_mutex_.lock();
        this->source_->read_async(sp, &this->buffer_, 1)
          .wait([this](const service_provider & sp, size_t readed) {
            //std::cout << this << "->_udp_send::start " << syscall(SYS_gettid) << ": unlock\n";
              this->start_mutex_.unlock();
              if (1 == readed) {
                this->write_async(sp);
              }
              else {
              }
            },
            [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              this->start_mutex_.unlock();
              sp.unhandled_exception(ex);
            },
          sp);
      }

      std::shared_ptr<async_stream<udp_datagram>> stream()
      {
        return this->source_;
      }

    private:
      _udp_socket & owner_;
      service_provider sp_;
      std::shared_ptr<async_stream<udp_datagram>> source_;
      socklen_t addr_len_;
      udp_datagram buffer_;


      void write_async(const service_provider & sp)
      {
        this->sp_ = sp;

#ifdef _WIN32
        this->wsa_buf_.len = this->buffer_.data_size();
        this->wsa_buf_.buf = (CHAR *)this->buffer_.data();

        if (NOERROR != WSASendTo(this->owner_.handle(), &this->wsa_buf_, 1, NULL, 0, (const sockaddr *)this->buffer_->addr(), sizeof(sockaddr_in), &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
#else
        if (nullptr == this->event_) {
          this->event_ = event_new(
            static_cast<_network_service *>(sp.get<inetwork_service>())->base_,
            this->owner_.handle(),
            EV_WRITE,
            &callback,
            this);
        }
        // Schedule client event
        event_add(this->event_, NULL);

        static_cast<_network_service *>(sp.get<inetwork_service>())->start_libevent_dispatch(sp);
#endif
      }
#ifdef _WIN32
      void process(DWORD dwBytesTransfered) override
      {
        if (this->wsa_buf_.len != (size_t)dwBytesTransfered) {
          throw std::runtime_error("Invalid sent UDP data");
        }

        this->source_->read_async(this->sp_, &this->buffer_, 1)
          .wait([this](const service_provider & sp, size_t readed) {
          if (1 == readed) {
            this->write_async(sp);
          }
          else {
          }
        },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        },
          this->sp_);
      }
      void error(DWORD error_code) override
      {
      }
#else
      struct event * event_;
      not_mutex event_mutex_;

      static void callback(int fd, short event, void *arg)
      {
        auto pthis = reinterpret_cast<_udp_send *>(arg);
        std::unique_lock<not_mutex> lock(pthis->event_mutex_);
        
        try {
          int len = sendto(fd,
            pthis->buffer_.data(),
            pthis->buffer_.data_size(),
            0,
            (sockaddr *)pthis->buffer_->addr(),
            sizeof(sockaddr_in));
          
          if (len < 0) {
            int error = errno;
            throw std::system_error(
              error,
              std::generic_category(),
              "Send to " + network_service::to_string(*pthis->buffer_->addr()));
          }
          
          if((size_t)len != pthis->buffer_.data_size()){
            throw std::runtime_error("Invalid send UDP");
          }

          pthis->sp_.get<logger>()->debug(pthis->sp_, "UDP Sent %d bytes to %s", len, network_service::to_string(*pthis->buffer_->addr()).c_str());
          pthis->start(pthis->sp_);
        }
        catch (const std::exception & ex) {
          pthis->sp_.unhandled_exception(std::make_shared<std::exception>(ex));
        }
        catch (...) {
          pthis->sp_.unhandled_exception(std::make_shared<std::runtime_error>("Unexpected error"));
        }
      }
#endif//_WIN32
    };

    _udp_receive receive_;
    _udp_send send_;
  };


  class _udp_server
  {
  public:
    _udp_server(
      const std::string & address,
      int port
    ) : address_(address), port_(port)
    {
    }

    udp_socket start(const service_provider & sp)
    {
      auto scope = sp.create_scope(("UDP server on " + this->address_ + ":" + std::to_string(this->port_)).c_str());
      imt_service::enable_async(scope);

      this->socket_->create(scope);
      
      sockaddr_in addr;
      memset((char *)&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(this->port_);
      addr.sin_addr.s_addr = htonl(INADDR_ANY);

      if (0 > bind(this->socket_->handle(), (sockaddr *)&addr, sizeof(addr))) {
#ifdef _WIN32
        auto error = WSAGetLastError();
#else
        auto error = errno;
#endif
        throw std::system_error(error, std::system_category(), "bind socket");
      }

      this->socket_->start(scope);
      return this->socket_;
    }

    void stop()
    {
    }

  private:
    udp_socket socket_;
    std::string address_;
    int port_;
  };


  class _udp_client
  {
  public:
    _udp_client()
    {

    }

    ~_udp_client()
    {

    }

    udp_socket start(const service_provider & sp)
    {
      this->socket_->create(sp);

      sockaddr_in addr;
      memset((char *)&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(0);
      addr.sin_addr.s_addr = htonl(INADDR_ANY);

      if (0 > bind(this->socket_->handle(), (sockaddr *)&addr, sizeof(addr))) {
#ifdef _WIN32
        auto error = WSAGetLastError();
#else
        auto error = errno;
#endif
        throw std::system_error(error, std::system_category(), "bind socket");
      }

      this->socket_->start(sp);
      return this->socket_;
    }


  private:
    udp_socket socket_;

  };
}

#endif//__VDS_NETWORK_UDP_SOCKET_P_H_