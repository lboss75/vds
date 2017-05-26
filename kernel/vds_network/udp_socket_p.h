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
      const sockaddr_in & addr,
      const void * data,
      size_t data_size)
      : addr_(addr), data_(data, data_size)
    {
    }

    const std::string & server() const { return network_service::get_ip_address_string(this->addr_); }
    uint16_t port() const { return ntohs(this->addr_.sin_port); }

    const void * data() const { return this->data_.data(); }
    size_t data_size() const { return this->data_.size(); }

    static udp_datagram create(const sockaddr_in & addr, const void * data, size_t data_size)
    {
      return udp_datagram(new _udp_datagram(addr, data, data_size));
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
        close(this->s_);
        throw std::system_error(error, std::system_category(), "Allow socket descriptor to be reuseable");
      }

      /*************************************************************/
      /* Set socket to be nonblocking. All of the sockets for    */
      /* the incoming connections will also be nonblocking since  */
      /* they will inherit that state from the listening socket.   */
      /*************************************************************/
      if (0 > ioctl(this->s_, FIONBIO, (char *)&on)) {
        auto error = errno;
        close(this->s_);
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

    async_stream<udp_datagram> & incoming()
    {
      return this->receive_.stream();
    }

    async_stream<udp_datagram> & outgoing()
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
        sp_(service_provider::empty())
#ifndef _WIN32
        , event_(nullptr)
        , network_service_(static_cast<network_service *>(args.sp_.get<inetwork_manager>()))
#endif
      {
      }

      void start(const service_provider & sp)
      {

      }

      async_stream<udp_datagram> & stream()
      {
        return this->target_;
      }

    private:
      _udp_socket & owner_;
      async_stream<udp_datagram> target_;
      service_provider sp_;
      sockaddr_in addr_;
      socklen_t addr_len_;
      uint8_t buffer_[10 * 1024 * 1024];
      size_t data_len_;

      void read_async(const vds::service_provider & sp)
      {
        this->sp_ = sp;

#ifdef _WIN32
        this->addr_len_ = sizeof(sockaddr_in);

        this->wsa_buf_.len = sizeof(this->buffer_);
        this->wsa_buf_.buf = (CHAR *)this->buffer_;

        DWORD flags = 0;
        DWORD numberOfBytesRecvd;
        if (NOERROR != WSARecvFrom(
          this->s_,
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
        this->addr_ = (struct sockaddr *)data;
        data += sizeof(sockaddr_in);
        this->addr_len_ = (socklen_t *)data;
        *this->addr_len_ = sizeof(sockaddr_in);
        data += sizeof(socklen_t);

        this->data_len_ = data_size - sizeof(sockaddr_in) - sizeof(socklen_t);
        this->data_ = data;

        if (nullptr == this->event_) {
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

#ifdef _WIN32
      void process(DWORD dwBytesTransfered) override
      {
        auto data = new udp_datagram[1];
        data[0] = _udp_datagram::create(this->addr_, this->buffer_, dwBytesTransfered);
        this->target_.write_all_async(this->sp_, data, 1)
          .wait([this, data](const service_provider & sp) {
          this->read_async(sp);
          delete data;
        },
            [data](const service_provider & sp, std::exception_ptr ex) {
          delete data;
        },
          this->sp_);
      }

      void error(DWORD error_code) override
      {
      }

#endif


#ifndef _WIN32
      network_socket::SOCKET_HANDLE s_;
      struct event * event_;
      network_service * network_service_;


      static void callback(int fd, short event, void *arg)
      {
        auto pthis = reinterpret_cast<handler *>(arg);

        int len = recvfrom(fd, pthis->data_, pthis->data_len_, 0,
          pthis->addr_, pthis->addr_len_);

        if (len < 0) {
          int error = errno;
          throw std::system_error(error, std::system_category(), "recvfrom");
        }

        pthis->buffer_.queue(pthis->sp_, (uint32_t)(len + sizeof(sockaddr_in) + sizeof(socklen_t)));
      }
#endif//_WIN32
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
        sp_(service_provider::empty())
      {

      }


      void start(const service_provider & sp)
      {
        this->source_.read_async(sp, &this->buffer_, 1)
          .wait([this](const service_provider & sp, size_t readed) {
          if (1 == readed) {
            this->write_async(sp);
          }
          else {
          }
        },
            [](const service_provider & sp, std::exception_ptr ex) {
        },
          sp);
      }

      async_stream<udp_datagram> & stream()
      {
        return this->source_;
      }

    private:
      _udp_socket & owner_;
      async_stream<udp_datagram> source_;
      service_provider sp_;
      socklen_t addr_len_;
      udp_datagram buffer_;


      void write_async(const service_provider & sp)
      {
        this->sp_ = sp;

#ifdef _WIN32
        this->wsa_buf_.len = this->buffer_.data_size();
        this->wsa_buf_.buf = (CHAR *)this->buffer_.data();

        sockaddr_in addr;
        memset((char *)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(this->buffer_.port());
        addr.sin_addr.s_addr = inet_addr(this->buffer_.server().c_str());;

        if (NOERROR != WSASendTo(this->owner_.handle(), &this->wsa_buf_, 1, NULL, 0, (const sockaddr *)&addr, sizeof(addr), &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
#else
        this->to_ = to;
        this->data_ = (const unsigned char *)data;
        this->data_size_ = len;

        if (nullptr == this->event_) {
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
        if (this->wsa_buf_.len != (size_t)dwBytesTransfered) {
          throw std::runtime_error("Invalid sent UDP data");
        }

        this->source_.read_async(this->sp_, &this->buffer_, 1)
          .wait([this](const service_provider & sp, size_t readed) {
          if (1 == readed) {
            this->write_async(sp);
          }
          else {
          }
        },
            [](const service_provider & sp, std::exception_ptr ex) {
        },
          this->sp_);
      }
      void error(DWORD error_code) override
      {
      }
#endif


#ifndef _WIN32
      const sockaddr_in * to_;
      const unsigned char * data_;
      size_t data_size_;
      network_service * network_service_;

      static void callback(int fd, short event, void *arg)
      {
        auto pthis = reinterpret_cast<handler *>(arg);
        try {
          logger::get(pthis->sp_)->trace(pthis->sp_,
            "Send %d bytes to %s", pthis->data_size_, network_service::to_string(*pthis->to_).c_str());

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
              [pthis]() { pthis->prev(pthis->sp_); });
          }
        }
        catch (...) {
          pthis->error(pthis->sp_, std::current_exception());
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
      this->socket_->start(sp);
      return this->socket_;
    }


  private:
    udp_socket socket_;

  };
}

#endif//__VDS_NETWORK_UDP_SOCKET_P_H_