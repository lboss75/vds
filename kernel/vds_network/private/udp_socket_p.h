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
      : s_(s)
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

    void start(
        const vds::service_provider & sp,
        const std::function<void(const udp_datagram &)> & target)
    {
#ifdef _WIN32
      this->reader_ = std::make_shared<_udp_receive>(sp, this->shared_from_this());
      this->reader_->start();
      
      this->writter_ = std::make_shared<_udp_send>(sp, this->shared_from_this());
      this->writter_->start();
#else
      auto handler = std::make_shared<_udp_handler>(sp, this->shared_from_this(), target);
      this->handler_ = handler;
      handler->start();
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
      auto handler = this->handler_.lock();
      this->handler_.reset();
      handler->stop();
#endif
    }

    async_task<const udp_datagram &> read_async();
    async_task<> write_async(const udp_datagram & message);


  private:
    SOCKET_HANDLE s_;

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
          .execute(
            [pthis](const std::shared_ptr<std::exception> & ex) {
				if(!ex){
					static_cast<_udp_receive *>(pthis.get())->read_async();
				}
				else {
					static_cast<_udp_receive *>(pthis.get())->sp_.unhandled_exception(ex);
				}
            });
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
			.execute([sp, this](const std::shared_ptr<std::exception> & ex, size_t readed) {
			if (!ex) {
				if (1 == readed) {
					this->schedule();
				}
				else {
					this->pthis_.reset();
				}
			}
			else {
				sp.unhandled_exception(ex);
			}
		});
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
    class _udp_handler : public _socket_task_impl<_udp_handler>
    {
      using this_class = _udp_handler;
    public:
      _udp_handler(
        const service_provider & sp,
        const std::shared_ptr<_udp_socket> & owner,
        const std::function<void(const udp_datagram &)> & target)
        : _socket_task_impl<_udp_handler>(sp, owner->s_),
          owner_(owner),
          write_result_([](const std::shared_ptr<std::exception> & ){
            throw std::runtime_error("Logic error");
          })
      {
      }
      
      ~_udp_handler()
      {
      }


      async_task<> write_async(const udp_datagram & message){

        std::lock_guard<std::mutex> lock(this->write_mutex_);
        switch (this->write_status_){
          case write_status_t::bof:
            this->write_message_ = message;
            return [pthis = this->shared_from_this()](const async_result<> & result){
              auto this_ = static_cast<_udp_handler *>(pthis.get());
              this_->write_result_ = result;
              this_->write_status_ = write_status_t::waiting_socket;
              this_->change_mask(EPOLLOUT);
            };

          case write_status_t::eof:
          case write_status_t::waiting_socket:
            throw  std::runtime_error("Invalid operator");
        }
      }

      void write_data() {
        std::unique_lock<std::mutex> lock(this->write_mutex_);

        if(write_status_t::waiting_socket != this->write_status_) {
          throw std::runtime_error("Invalid operation");
        }

          auto size = this->write_message_.data_size();
          int len = sendto(
            this->s_,
            this->write_message_.data(),
            size,
            0,
            (sockaddr *)this->write_message_->addr(),
            sizeof(sockaddr_in));
          
          if (len < 0) {
            int error = errno;
            this->write_status_ = write_status_t::bof;
            lock.unlock();

            this->write_result_.error(
                std::make_shared<std::system_error>(
                  error,
                  std::generic_category(),
                  "Send to " + network_service::to_string(*this->write_message_->addr())));
          }
        
          if((size_t)len != size){
            throw std::runtime_error("Invalid send UDP");
          }
          lock.unlock();
          this->sp_.get<logger>()->trace(
              "UDP",
              this->sp_,
              "Sent %d bytes to %s",
              len,
              network_service::to_string(*this->write_message_->addr()).c_str());

          this->write_result_.done();
        }

      void read_data() {
        for(;;) {
          this->addr_len_ = sizeof(this->addr_);
          int len = recvfrom(this->s_,
                             this->read_buffer_,
                             sizeof(this->read_buffer_),
                             0,
                             (struct sockaddr *)&this->addr_,
                             &this->addr_len_);

          if (len < 0) {
            int error = errno;
            if(EAGAIN == error){
              break;
            }

            throw std::system_error(error, std::system_category(), "recvfrom");
          }

          this->sp_.get<logger>()->trace("UDP", this->sp_, "got %d bytes from %s", len, network_service::to_string(this->addr_).c_str());
          this->target_(_udp_datagram::create(this->addr_, this->read_buffer_, len));
        }
      }

    private:
      std::shared_ptr<_udp_socket> owner_;
      async_result<> write_result_;
      std::function<void(const udp_datagram &)> target_;

      sockaddr_in addr_;
      socklen_t addr_len_;
      uint8_t read_buffer_[64 * 1024];
      
      udp_datagram write_message_;
    };

    std::weak_ptr<_udp_handler> handler_;
#endif//_WIN32
#ifdef _WIN32
    std::shared_ptr<_udp_receive> reader_;
    std::shared_ptr<_udp_send> writter_;
#else
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

    udp_socket start(const service_provider & sp, const std::function<void(const udp_datagram &)> & target)
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

      this->socket_->start(scope, target);
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

    udp_socket start(const service_provider & sp, const std::function<void(const udp_datagram &)> & target)
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

      this->socket_->start(sp, target);
      return this->socket_;
    }


  private:
    udp_socket socket_;

  };
}

#endif//__VDS_NETWORK_UDP_SOCKET_P_H_
