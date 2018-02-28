#ifndef __VDS_NETWORK_UDP_SOCKET_P_H_
#define __VDS_NETWORK_UDP_SOCKET_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <udp_datagram_size_exception.h>
#include "network_types_p.h"
#include "service_provider.h"
#include "network_service_p.h"
#include "udp_socket.h"
#include "socket_task_p.h"
#include "const_data_buffer.h"
#include "leak_detect.h"

namespace vds {

  class _udp_datagram
  {
  public:
    _udp_datagram(
      const network_address & address,
      const void * data,
      size_t data_size)
      : address_(address),
        data_(data, data_size)
    {
    }

    _udp_datagram(
      const network_address & address,
      const const_data_buffer & data)
      : address_(address),
        data_(data)
    {
    }

    const network_address & address() const { return this->address_; }
    
    const uint8_t * data() const { return this->data_.data(); }
    size_t data_size() const { return this->data_.size(); }

    static udp_datagram create(const network_address & addr, const void * data, size_t data_size)
    {
      return udp_datagram(new _udp_datagram(addr, data, data_size));
    }

    static udp_datagram create(const network_address & addr, const const_data_buffer & data)
    {
      return udp_datagram(new _udp_datagram(addr, data));
    }

  private:
    network_address address_;
    const_data_buffer data_;
  };


  class _udp_socket : public std::enable_shared_from_this<_udp_socket>
  {
  public:
    _udp_socket(SOCKET_HANDLE s = INVALID_SOCKET)
      : s_(s)
#ifndef _WIN32
      , broadcast_enabled_(false)
#endif
    {
      this->leak_detect_.name_ = "_udp_socket";
      this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector){
#ifdef _WIN32
		  collector->add(this->reader_);
		  collector->add(this->writter_);
#else
		  collector->add(this->handler_);
#endif
      };
    }

    ~_udp_socket()
    {
      this->close();
    }

    SOCKET_HANDLE handle() const
    {
      return this->s_;
    }

    void start(const vds::service_provider & sp)
    {
#ifdef _WIN32
      this->reader_ = std::make_shared<_udp_receive>(sp, this->s_);
      this->writter_ = std::make_shared<_udp_send>(sp, this->s_);
#else
      auto handler = std::make_shared<_udp_handler>(sp, this->shared_from_this());
      this->handler_ = handler;
      handler->start();
#endif
    }

    void prepare_to_stop(const service_provider & sp)
    {
#ifdef _WIN32
		this->reader_->prepare_to_stop(sp);
		this->writter_->prepare_to_stop(sp);
#else
		this->handler_->prepare_to_stop(sp);
#endif// _WIN32
    }

    void stop()
    {
#ifdef _WIN32
      this->reader_.reset();
      this->writter_.reset();
#else
      this->handler_->stop();
      this->handler_.reset();
#endif
    }

    async_task<const udp_datagram &> read_async()
    {
#ifdef _WIN32
		if (!this->reader_) {
			return async_task<const udp_datagram &>(std::make_shared<std::system_error>(
				ECONNRESET, std::system_category(), "Socket is closed"));
		}
		return this->reader_->read_async();
#else
      if(!this->handler_){
        throw std::system_error(ECONNRESET, std::system_category(), "Socket is closed");
      }
      return this->handler_->read_async();
#endif
    }

    async_task<> write_async(const udp_datagram & message)
    {
#ifdef _WIN32
      return this->writter_->write_async(message);
#else
      return this->handler_->write_async(message);
#endif
    }

    void send_broadcast(int port, const const_data_buffer & message)
    {
#ifndef _WIN32
      if(!this->broadcast_enabled_) {
        int broadcastEnable = 1;
        setsockopt(this->s_, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
        this->broadcast_enabled_ = true;
      }
#endif

      struct sockaddr_in s;
      memset(&s, 0, sizeof(struct sockaddr_in));

      s.sin_family = AF_INET;
      s.sin_port = htons(port);
      s.sin_addr.s_addr = htonl(INADDR_BROADCAST);

      if(0 > sendto(
          this->s_,
          (const char *)message.data(),
          message.size(),
          0,
          (struct sockaddr *)&s,
          sizeof(struct sockaddr_in))) {
        int error = errno;

        if(EMSGSIZE == error){
          throw udp_datagram_size_exception();
        }

        throw std::system_error(error, std::system_category(), "send broadcast");
      }
    }

  private:
#ifndef _WIN32
    bool broadcast_enabled_;
#endif
    SOCKET_HANDLE s_;

    void close()
    {
#ifdef _WIN32
      if (INVALID_SOCKET != this->s_) {
		  std::cout << "vds::_udp_socket::close\n";

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
        SOCKET_HANDLE s)
        : sp_(sp),
          s_(s),
          result_([](const std::shared_ptr<std::exception> &, const udp_datagram &) {
            throw std::runtime_error("Design error");
          })
      {
      }

      async_task<const udp_datagram &> read_async()
      {
        return [pthis = this->shared_from_this()](const async_result<const udp_datagram &> & result){
          auto this_ = static_cast<_udp_receive *>(pthis.get());
          this_->wsa_buf_.len = sizeof(this_->buffer_);
          this_->wsa_buf_.buf = (CHAR *)this_->buffer_;
          this_->result_ = result;
          
		  this_->sp_.get<logger>()->trace("UDP", this_->sp_, "WSARecvFrom %d", this_->s_);

          DWORD flags = 0;
          DWORD numberOfBytesRecvd;
          if (NOERROR != WSARecvFrom(
            this_->s_,
            &this_->wsa_buf_,
            1,
            &numberOfBytesRecvd,
            &flags,
            this_->addr_,
            this_->addr_.size_ptr(),
            &this_->overlapped_,
            NULL)) {
            auto errorCode = WSAGetLastError();
            if (WSA_IO_PENDING != errorCode) {
              result.error(std::make_shared<std::system_error>(errorCode, std::system_category(), "WSARecvFrom failed"));
            }
			else {
				this_->sp_.get<logger>()->trace("UDP", this_->sp_, "Read scheduled");
			}
          }
		  else {
			  auto errorCode = WSAGetLastError();
			  this_->sp_.get<logger>()->trace("UDP", this_->sp_, "Direct readed %d, code %d", numberOfBytesRecvd, errorCode);
			  //this_->process(numberOfBytesRecvd);
		  }
        };
      }

	  void prepare_to_stop(const service_provider & sp)
	  {
	  }

    private:
      service_provider sp_;
      SOCKET_HANDLE s_;
      async_result<const udp_datagram &> result_;

      network_address addr_;
      uint8_t buffer_[64 * 1024];

      void process(DWORD dwBytesTransfered) override
      {
        auto pthis = this->shared_from_this();
		    this->sp_.get<logger>()->trace("UDP", this->sp_, "Readed %d", dwBytesTransfered);
        this->result_.done(_udp_datagram::create(this->addr_, this->buffer_, (size_t)dwBytesTransfered));
      }

      void error(DWORD error_code) override
      {
        this->result_.error(std::make_shared<std::system_error>(error_code, std::system_category(), "WSARecvFrom failed"));
      }
    };

    class _udp_send : public _socket_task
    {
    public:
      _udp_send(
        const service_provider & sp,
        SOCKET_HANDLE s)
        : sp_(sp),
          s_(s),
          result_([](const std::shared_ptr<std::exception> &) {
          throw std::runtime_error("Design error");
        })
      {
		  this->leak_detect_.name_ = "_udp_send";
      }

        async_task<> write_async(const udp_datagram & data)
        {
          this->buffer_ = data;
          this->wsa_buf_.len = this->buffer_.data_size();
          this->wsa_buf_.buf = (CHAR *)this->buffer_.data();

          return[pthis = this->shared_from_this()](const async_result<> & result){
            auto this_ = static_cast<_udp_send *>(pthis.get());
            this_->result_ = result;

			this_->sp_.get<logger>()->trace("UDP", this_->sp_, "WSASendTo");
			if (NOERROR != WSASendTo(this_->s_, &this_->wsa_buf_, 1, NULL, 0, this_->buffer_->address(), this_->buffer_->address().size(), &this_->overlapped_, NULL)) {
              auto errorCode = WSAGetLastError();
              if (WSA_IO_PENDING != errorCode) {
                result.error(std::make_shared<std::system_error>(errorCode, std::system_category(), "WSASend failed"));
              }
            }
          };
        }

		void prepare_to_stop(const service_provider & sp)
		{
		}

    private:
      service_provider sp_;
      SOCKET_HANDLE s_;
      async_result<> result_;

      socklen_t addr_len_;
      udp_datagram buffer_;

      void process(DWORD dwBytesTransfered) override
      {
        if (this->wsa_buf_.len != (size_t)dwBytesTransfered) {
          this->result_.error(std::make_shared<std::runtime_error>("Invalid sent UDP data"));
        }
        else {
			this->sp_.get<logger>()->trace("UDP", this->sp_, "Sent %d", dwBytesTransfered);
			this->result_.done();
        }
      }

      void error(DWORD error_code) override
      {
        this->result_.error(std::make_shared<std::system_error>(error_code, std::system_category(), "WSASendTo failed"));
      }
	};

#else
    class _udp_handler : public _socket_task_impl<_udp_handler>
    {
      using this_class = _udp_handler;
      using base_class = _socket_task_impl<_udp_handler>;
    public:
      _udp_handler(
        const service_provider & sp,
        const std::shared_ptr<_udp_socket> & owner)
        : _socket_task_impl<_udp_handler>(sp, owner->s_),
          owner_(owner),
          read_result_([](const std::shared_ptr<std::exception> &, const udp_datagram & ){
            throw std::runtime_error("Logic error");
          }),
          write_result_([](const std::shared_ptr<std::exception> & ){
            throw std::runtime_error("Logic error");
          })
      {
        this->leak_detect_.name_ = "_udp_handler";
        this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector){
          collector->add(this->owner_);
        };
      }
      
      ~_udp_handler()
      {
      }

      async_task<const udp_datagram &> read_async() {
        std::lock_guard<std::mutex> lock(this->read_mutex_);
        switch (this->read_status_) {
          case read_status_t::bof:
            return [pthis = this->shared_from_this()](
                const async_result<const udp_datagram &> & result){
              auto this_ = static_cast<_udp_handler *>(pthis.get());
              this_->read_result_ = result;
              this_->read_status_ = read_status_t::waiting_socket;
              this_->change_mask(EPOLLIN);
            };
          case read_status_t::continue_read:
            return [pthis = this->shared_from_this()](
                const async_result<const udp_datagram &> & result){
              auto this_ = static_cast<_udp_handler *>(pthis.get());
              this_->read_result_ = result;
              this_->read_data();
            };

          case read_status_t::eof:
            return async_task<const udp_datagram &>(
                std::make_shared<std::system_error>(ECONNRESET, std::system_category())
            );

          default:
            throw  std::runtime_error("Invalid operator");
        }
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
            return async_task<>(
                std::make_shared<std::system_error>(
                    ECONNRESET, std::system_category()));

          case write_status_t::waiting_socket:
          default:
            throw  std::runtime_error("Invalid operator");
        }
      }

      void write_data() {
        std::unique_lock<std::mutex> lock(this->write_mutex_);

        if(write_status_t::eof == this->write_status_) {
          this->write_result_.error(
              std::make_shared<std::system_error>(
                  ECONNRESET,
                  std::generic_category()));
          return;
        }

        if(write_status_t::waiting_socket != this->write_status_) {
          throw std::runtime_error("Invalid operation");
        }

          auto size = this->write_message_.data_size();
          int len = sendto(
            this->s_,
            this->write_message_.data(),
            size,
            0,
            this->write_message_->address(),
            sizeof(sockaddr_in));

          this->write_status_ = write_status_t::bof;
          if (len < 0) {
            int error = errno;
            lock.unlock();

            if(EMSGSIZE == error){
              this->write_result_.error(
                  std::make_shared<udp_datagram_size_exception>());
            }
            else {
              this->write_result_.error(
                  std::make_shared<std::system_error>(
                      error,
                      std::generic_category(),
                      "Send to " + this->write_message_->address().to_string()));
            }
            return;
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
              this->write_message_->address().to_string().c_str());

          this->write_result_.done();
        }

      void read_data() {
        std::unique_lock<std::mutex> lock(this->read_mutex_);

        if(read_status_t::eof == this->read_status_){
          auto result = std::move(this->read_result_);
          lock.unlock();

          if(result) {
            result.error(std::make_shared<std::system_error>(ECONNRESET, std::system_category()));
          }
          return;
        }

        if(read_status_t::waiting_socket != this->read_status_
            && read_status_t::continue_read != this->read_status_) {
          throw std::runtime_error("Invalid operation");
        }

        this->addr_.reset();
        int len = recvfrom(this->s_,
                           this->read_buffer_,
                           sizeof(this->read_buffer_),
                           0,
                           this->addr_,
                           this->addr_.size_ptr());

        if (len < 0) {
          int error = errno;
          if(EAGAIN == error){
            this->read_status_ = read_status_t::waiting_socket;
            this->change_mask(EPOLLIN);
            return;
          }

          auto result = std::move(this->read_result_);
          lock.unlock();

          result.error(std::make_shared<std::system_error>(error, std::system_category(), "recvfrom"));
        }
        else {
          this->sp_.get<logger>()->trace("UDP", this->sp_, "got %d bytes from %s", len,
                                         this->addr_.to_string().c_str());
          this->read_status_ = read_status_t::continue_read;
          auto result = std::move(this->read_result_);
          lock.unlock();

          result.done(_udp_datagram::create(this->addr_, this->read_buffer_, len));
        }
      }

      void prepare_to_stop(const service_provider & sp)
      {
        std::unique_lock<std::mutex> lock1(this->write_mutex_);
        std::unique_lock<std::mutex> lock2(this->read_mutex_);

        if(this->read_result_) {
          this->read_result_.error(std::make_shared<std::system_error>(ECONNRESET, std::system_category()));
        }

        if(this->write_result_) {
          this->write_result_.error(std::make_shared<std::system_error>(ECONNRESET, std::system_category()));
        }

        this->read_status_ = read_status_t::eof;
        this->write_status_ = write_status_t::eof;
      }

      void stop(){
        base_class::stop();

        this->read_result_.clear();
        this->write_result_.clear();

        this->owner_.reset();
      }

    private:
      std::shared_ptr<_udp_socket> owner_;
      async_result<const udp_datagram &> read_result_;
      async_result<> write_result_;

      network_address addr_;
      uint8_t read_buffer_[64 * 1024];
      
      udp_datagram write_message_;

    public:
      leak_detect_helper leak_detect_;
    };

    std::shared_ptr<_udp_handler> handler_;
#endif//_WIN32
#ifdef _WIN32
    std::shared_ptr<_udp_receive> reader_;
    std::shared_ptr<_udp_send> writter_;
#else
#endif
  public:
    leak_detect_helper leak_detect_;
  };

  class _udp_server
  {
  public:
    _udp_server(const network_address & address)
        : address_(address)
    {
      this->leak_detect_.name_ = "_udp_server";
      this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector){
        collector->add(this->socket_);
      };
    }

    udp_socket & start(const service_provider & sp)
    {
      auto scope = sp.create_scope(("UDP server on " + this->address_.to_string()).c_str());
      imt_service::enable_async(scope);

      this->socket_ = udp_socket::create(scope, this->address_.family());
      
      if (0 > bind(this->socket_->handle(), this->address_, this->address_.size())) {
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

    void prepare_to_stop(const service_provider & sp)
    {
      this->socket_->prepare_to_stop(sp);
    }

    void stop(const service_provider & sp)
    {
      this->socket_->stop();
    }

    udp_socket & socket() { return this->socket_; }
	const udp_socket & socket() const { return this->socket_; }

  private:
    udp_socket socket_;
    network_address address_;

  public:
    leak_detect_helper leak_detect_;
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

    udp_socket & start(const service_provider & sp, sa_family_t af)
    {
      this->socket_ = udp_socket::create(sp, af);

      /*
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
      */
      this->socket_->start(sp);
      return this->socket_;
    }


  private:
    udp_socket socket_;

  };
}

#endif//__VDS_NETWORK_UDP_SOCKET_P_H_
