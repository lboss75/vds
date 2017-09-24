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


  class _udp_socket : public std::enable_shared_from_this<_udp_socket>
  {
  public:
    _udp_socket(SOCKET_HANDLE s = INVALID_SOCKET)
      : s_(s),
      incoming_(new continuous_stream<udp_datagram>()),
      outgoing_(new async_stream<udp_datagram>())
    {
    }

    ~_udp_socket()
    {
      this->close();
    }

    SOCKET_HANDLE handle() const
    {
      return this->s_;
    }

    std::shared_ptr<continuous_stream<udp_datagram>> incoming()
    {
      return this->incoming_;
    }

    std::shared_ptr<async_stream<udp_datagram>> outgoing()
    {
      return this->outgoing_;
    }

    void start(const vds::service_provider & sp)
    {
#ifdef _WIN32
      this->reader_ = std::make_shared<_udp_receive>(sp, this->shared_from_this());
      this->reader_->start();
      
      this->writter_ = std::make_shared<_udp_send>(sp, this->shared_from_this());
      this->writter_->start();
#else
      this->handler_ = std::make_shared<_udp_handler>(sp, this->shared_from_this());
      this->handler_->start();
#endif
    }
    
    void stop()
    {
#ifdef _WIN32
      this->reader_->stop();
      this->writter_->stop();
      
      this->reader_.reset();
      this->writter_.reset();
#else
      this->handler_->stop();
      this->handler_.reset();
#endif
    }

  private:
    SOCKET_HANDLE s_;
    std::shared_ptr<continuous_stream<udp_datagram>> incoming_;
    std::shared_ptr<async_stream<udp_datagram>> outgoing_;
    
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

#ifdef _WIN32
    class _udp_receive : public _socket_task
    {
    public:
      _udp_receive(
        const service_provider & sp,
        const std::shared_ptr<_udp_socket> & owner)
        : sp_(sp),
          owner_(owner)
      {
      }

      void start()
      {
        this->read_async();
      }

      void stop()
      {
      }

      void check_timeout(const service_provider & sp) override
      {
      }

      void prepare_to_stop(const service_provider & sp) override
      {
      }

    private:
      service_provider sp_;
      std::shared_ptr<_udp_socket> owner_;
      std::shared_ptr<_socket_task> pthis_;

      sockaddr_in addr_;
      socklen_t addr_len_;
      uint8_t buffer_[10 * 1024 * 1024];

      void read_async()
      {
        this->addr_len_ = sizeof(sockaddr_in);

        this->wsa_buf_.len = sizeof(this->buffer_);
        this->wsa_buf_.buf = (CHAR *)this->buffer_;

        DWORD flags = 0;
        DWORD numberOfBytesRecvd;
        if (NOERROR != WSARecvFrom(
          this->owner_->s_,
          &this->wsa_buf_,
          1,
          &numberOfBytesRecvd,
          &flags,
          (struct sockaddr *)&this->addr_,
          &this->addr_len_,
          &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            if (INVALID_SOCKET == this->owner_->s_) {
              return;
            }

            throw std::system_error(errorCode, std::system_category(), "WSARecvFrom failed");
          }
        }

        this->pthis_ = this->shared_from_this();
      }

      void process(DWORD dwBytesTransfered) override
      {
        auto pthis = this->shared_from_this();
        this->pthis_.reset();

        this->owner_->incoming_->write_value_async(
          this->sp_,
          _udp_datagram::create(this->addr_, this->buffer_, (size_t)dwBytesTransfered))
          .wait(
            [pthis](const service_provider & sp) {
              static_cast<_udp_receive *>(pthis.get())->read_async();
            },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            },
            this->sp_);
      }
      void error(DWORD error_code) override
      {
        switch (error_code)
        {
        case WSAESHUTDOWN:
        case ERROR_OPERATION_ABORTED:
        case ERROR_PORT_UNREACHABLE:
          break;
        default:
          this->sp_.unhandled_exception(std::make_shared<std::system_error>(error_code, std::system_category(), "WSARecvFrom failed"));
        }
      }
    };

    class _udp_send : public _socket_task
    {
    public:
      _udp_send(
        const service_provider & sp,
        const std::shared_ptr<_udp_socket> & owner)
        : sp_(sp),
          owner_(owner)
      {        
      }

      ~_udp_send()
      {
        this->sp_.get<logger>()->trace("UDP", this->sp_, "_udp_send.~_udp_send");
      }

      void start()
      {
        this->pthis_ = this->shared_from_this();
        this->write_async();
      }

      void stop()
      {
      }

      void check_timeout(const service_provider & sp) override
      {
      }

      void prepare_to_stop(const service_provider & sp) override
      {
      }

    private:
      service_provider sp_;
      std::shared_ptr<_udp_socket> owner_;
      std::shared_ptr<_socket_task> pthis_;

      socklen_t addr_len_;
      udp_datagram buffer_;

      void write_async()
      {
        auto sp = this->sp_.create_scope("_udp_send.write_async");
        this->owner_->outgoing_->read_async(sp, &this->buffer_, 1)
          .wait([this](const service_provider & sp, size_t readed) {
          if (1 == readed) {
            this->schedule();
          }
          else {
            this->pthis_.reset();
          }
        },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          sp.unhandled_exception(ex);
        },
          sp);
      }

      void schedule()
      {
        this->wsa_buf_.len = this->buffer_.data_size();
        this->wsa_buf_.buf = (CHAR *)this->buffer_.data();

        if (NOERROR != WSASendTo(this->owner_->s_, &this->wsa_buf_, 1, NULL, 0, (const sockaddr *)this->buffer_->addr(), sizeof(sockaddr_in), &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
      }

      void process(DWORD dwBytesTransfered) override
      {
        if (this->wsa_buf_.len != (size_t)dwBytesTransfered) {
          throw std::runtime_error("Invalid sent UDP data");
        }

        this->write_async();
      }

      void error(DWORD error_code) override
      {
        this->sp_.unhandled_exception(std::make_shared<std::system_error>(error_code, std::system_category(), "WSASendTo failed"));
      }
    };

#else
    class _udp_handler : public _socket_task
    {
      using this_class = _udp_handler;
    public:
      _udp_handler(
        const service_provider & sp,
        const std::shared_ptr<_udp_socket> & owner)
        : sp_(sp), owner_(owner),
          network_service_(static_cast<_network_service *>(this->sp_.get<inetwork_service>())),
          event_masks_(EPOLLIN | EPOLLET),
          read_timeout_ticks_(0),
          closed_(false)
      {
      }
      
      ~_udp_handler()
      {
      }

      void start()
      {
        this->network_service_->associate(this->sp_, this->owner_->s_, this->shared_from_this(), this->event_masks_);
        this->schedule_write();
      }
      
      void stop()
      {
        if(EPOLLET != this->event_masks_){
          this->network_service_->remove_association(this->sp_, this->owner_->s_);
        }
      }
      
      void process(uint32_t events) override
      {
        if(EPOLLOUT == (EPOLLOUT & events)){
          this->sp_.get<logger>()->trace("UDP", this->sp_, "UDP EPOLLOUT event");
          this->change_mask(0, EPOLLOUT);
          
          auto size = this->write_buffer_.data_size();
          int len = sendto(
            this->owner_->s_,
            this->write_buffer_.data(),
            size,
            0,
            (sockaddr *)this->write_buffer_->addr(),
            sizeof(sockaddr_in));
          
          if (len < 0) {
            int error = errno;
            throw std::system_error(
              error,
              std::generic_category(),
              "Send to " + network_service::to_string(*this->write_buffer_->addr()));
          }
        
          if((size_t)len != size){
            throw std::runtime_error("Invalid send UDP");
          }
          this->sp_.get<logger>()->trace("UDP", this->sp_, "Sent %d bytes to %s", len, network_service::to_string(*this->write_buffer_->addr()).c_str());
          this->schedule_write();
        }
        
        if(EPOLLIN == (EPOLLIN & events)){
          this->sp_.get<logger>()->trace("UDP", this->sp_, "EPOLLIN event");
          if(0 < this->owner_->s_){
            this->change_mask(0, EPOLLIN);
          }
          
          this->read_data();
        }
      }
      
      void check_timeout(const service_provider & sp) override
      {
        sp.get<logger>()->trace("UDP", sp, "check_timeout(ticks=%d, is_closed=%s)",
           this->read_timeout_ticks_,
           (this->closed_ ? "true" : "false"));
        
        if(1 < this->read_timeout_ticks_++ && !this->closed_){
          this->change_mask(0, EPOLLIN | EPOLLOUT);

          this->sp_.get<logger>()->trace("UDP", this->sp_, "read timeout");
          this->closed_ = true;
          this->owner_->incoming_->write_all_async(this->sp_, nullptr, 0)
          .wait(
            [](const service_provider & sp) {
              sp.get<logger>()->trace("UDP", sp, "input closed");
            },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            },
            this->sp_);
        }          
      }
      
      void prepare_to_stop(const service_provider & /*sp*/) override
      {
        if(!this->closed_){
          this->change_mask(0, EPOLLIN);

          this->sp_.get<logger>()->trace("UDP", this->sp_, "read timeout");
          this->closed_ = true;
          this->owner_->incoming_->write_all_async(this->sp_, nullptr, 0)
          .wait(
            [](const service_provider & sp) {
              sp.get<logger>()->trace("UDP", sp, "input closed");
            },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            },
            this->sp_);
        }          
      }


    private:
      service_provider sp_;
      std::shared_ptr<_udp_socket> owner_;
      _network_service * network_service_;
      
      std::mutex event_masks_mutex_;
      uint32_t event_masks_;
      
      sockaddr_in addr_;
      socklen_t addr_len_;
      uint8_t read_buffer_[10 * 1024 * 1024];
      
      udp_datagram write_buffer_;
      int read_timeout_ticks_;
      bool closed_;
      
      void change_mask(uint32_t set_events, uint32_t clear_events = 0)
      {
        std::unique_lock<std::mutex> lock(this->event_masks_mutex_);
        auto need_create = (EPOLLET == this->event_masks_);
        this->event_masks_ |= set_events;
        this->event_masks_ &= ~clear_events;
        
        if(!need_create && EPOLLET != this->event_masks_){
          this->network_service_->set_events(this->sp_, this->owner_->s_, this->event_masks_);
        }
        else if (EPOLLET == this->event_masks_){
          this->network_service_->remove_association(this->sp_, this->owner_->s_);
        }
        else {
          this->network_service_->associate(this->sp_, this->owner_->s_, this->shared_from_this(), this->event_masks_);
        }
      }
      
      void schedule_write()
      {
        this->owner_->outgoing_->read_async(this->sp_, &this->write_buffer_, 1)
        .wait([pthis = this->shared_from_this()](const service_provider & sp, size_t readed){
          if(0 == readed){
            //End of stream
            shutdown(static_cast<this_class *>(pthis.get())->owner_->s_, SHUT_WR);
          }
          else {
            static_cast<this_class *>(pthis.get())->change_mask(EPOLLOUT);
          }
        },
        [](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
          sp.unhandled_exception(ex);
        },
        this->sp_);
      }

      void read_data()
      {
        this->addr_len_ = sizeof(this->addr_);
        int len = recvfrom(this->owner_->s_, this->read_buffer_, sizeof(this->read_buffer_), 0,
          (struct sockaddr *)&this->addr_, &this->addr_len_);

        if (len < 0) {
          int error = errno;
          if(EAGAIN == error){
            this->change_mask(EPOLLIN);
            return;
          }
          throw std::system_error(error, std::system_category(), "recvfrom");
        }
        else {
          this->closed_ = true;
        }
        
        this->read_timeout_ticks_ = 0;
        this->sp_.get<logger>()->trace("UDP", this->sp_, "got %d bytes from %s", len, network_service::to_string(this->addr_).c_str());
        this->owner_->incoming_->write_value_async(this->sp_, _udp_datagram::create(this->addr_, this->read_buffer_, len))
          .wait(
            [pthis = this->shared_from_this(), len](const service_provider & sp) {
              if(0 != len){
                static_cast<this_class *>(pthis.get())->read_data();
              }
              else {
                sp.get<logger>()->trace("UDP", sp, "input closed");
              }
            },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            },
            this->sp_);
      }
    };
#endif//_WIN32
#ifdef _WIN32
    std::shared_ptr<_udp_receive> reader_;
    std::shared_ptr<_udp_send> writter_;
#else
    std::shared_ptr<_udp_handler> handler_;
#endif
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

      this->socket_ = udp_socket::create(scope);
      
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

    void stop(const service_provider & sp)
    {
      this->socket_->stop();
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
      this->socket_ = udp_socket::create(sp);

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