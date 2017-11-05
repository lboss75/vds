#ifndef __VDS_NETWORK_NETWORK_SOCKET_P_H_
#define __VDS_NETWORK_NETWORK_SOCKET_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_types_p.h"
#include "tcp_network_socket.h"
#include "socket_task_p.h"
#include "network_service_p.h"

namespace vds {
  class _network_service;

  class _tcp_network_socket : public _stream_async<uint8_t>
  {
  public:
    _tcp_network_socket()
    : s_(INVALID_SOCKET)
    {
    }

    _tcp_network_socket(SOCKET_HANDLE s)
      : s_(s)
    {
#ifdef _WIN32
      if (INVALID_SOCKET == s) {
#else
      if (s < 0) {
#endif
        throw std::runtime_error("Invalid socket handle");
      }
    }

    _tcp_network_socket(const _tcp_network_socket &) = delete;

    ~_tcp_network_socket()
    {
      this->close();
    }

    void close()
    {
#ifdef _WIN32
      if (INVALID_SOCKET != this->s_) {
        shutdown(this->s_, SD_BOTH);
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
      
    SOCKET_HANDLE handle() const {
#ifdef _WIN32
      if (INVALID_SOCKET == this->s_) {
#else
      if (0 >= this->s_) {
#endif
        throw std::logic_error("network_socket::handle without open socket");
      }
      return this->s_;
    }
    
    static tcp_network_socket from_handle(SOCKET_HANDLE handle)
    {
      return tcp_network_socket(new _tcp_network_socket(handle));
    }

    void start(const service_provider & sp)
    {
      sp.get<logger>()->trace("TCP", sp, "socket start");
      
#ifdef _WIN32
      std::make_shared<_read_socket_task>(sp, this->shared_from_this())->read_async();
      std::make_shared<_write_socket_task>(sp, this->shared_from_this())->write_async();
#else
      std::make_shared<_socket_handler>(sp, this->s_)->start();
#endif//_WIN32
    }

    async_task<size_t /*readed*/> read_async(uint8_t * data, size_t size){
      return static_cast<_socket_handler *>(this->socket_task_.get())->read_async(data, size);
    }
    async_task<> write_async(const uint8_t * data, size_t size) override {
      return static_cast<_socket_handler *>(this->socket_task_.get())->write_async(data, size);
    }

#ifndef _WIN32
    void make_socket_non_blocking()
    {
      auto flags = fcntl (this->handle(), F_GETFL, 0);
      if (flags == -1) {
        throw std::runtime_error("fcntl");
      }

      flags |= O_NONBLOCK;
      auto s = fcntl (this->handle(), F_SETFL, flags);
      if (s == -1) {
        throw std::runtime_error("fcntl");
      }
    }
    void set_timeouts()
    {
//       struct timeval tv;
//       tv.tv_sec = 30;        // 30 Secs Timeout
//       tv.tv_usec = 0;
//       setsockopt(this->handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
    }
#endif//_WIN32

  private:
    SOCKET_HANDLE s_;
#ifdef _WIN32
#else
    std::shared_ptr<_socket_task> socket_task_;
#endif

#ifdef _WIN32
    class _read_socket_task : public _socket_task
    {
    public:
      constexpr static size_t BUFFER_SIZE = 1024;

      _read_socket_task(
        const service_provider & sp,
        const std::shared_ptr<_tcp_network_socket> & owner)
        : owner_(owner),
        sp_(sp)
      {
      }

      ~_read_socket_task()
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv closed");
      }

      void read_async()
      {
        this->wsa_buf_.len = BUFFER_SIZE;
        this->wsa_buf_.buf = (CHAR *)this->buffer_;

        this->pthis_ = this->shared_from_this();

        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv");
        DWORD flags = 0;
        DWORD numberOfBytesRecvd;
        if (NOERROR != WSARecv(this->owner_->s_, &this->wsa_buf_, 1, &numberOfBytesRecvd, &flags, &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv error");
            auto pthis = this->pthis_;
            this->pthis_.reset();

            if (WSAESHUTDOWN == errorCode) {
              this->owner_->incoming_->write_async(this->sp_, nullptr, 0)
                .execute(
	                [pthis](const std::shared_ptr<std::exception> & error) {
				  if (error) {
					  static_cast<_read_socket_task *>(pthis.get())->sp_.unhandled_exception(error);
				  }
                });
            }
            else {
              this->sp_.unhandled_exception(std::make_unique<std::system_error>(errorCode, std::system_category(), "read from tcp socket"));
            }
            return;
          }
        }
      }

      void check_timeout(const service_provider & sp) override
      {
      }

      void prepare_to_stop(const service_provider & sp) override
      {
      }

    private:
      service_provider sp_;
      std::shared_ptr<_tcp_network_socket> owner_;
      std::shared_ptr<_socket_task> pthis_;
      uint8_t buffer_[BUFFER_SIZE];

      void process(DWORD dwBytesTransfered) override
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv got(%d)", dwBytesTransfered);
        auto pthis = this->pthis_;
        this->pthis_.reset();

        if (0 == dwBytesTransfered) {
          this->owner_->incoming_->write_async(this->sp_, nullptr, 0)
			  .execute([pthis](const std::shared_ptr<std::exception> & error) {
			  if (error) {
				  static_cast<_read_socket_task *>(pthis.get())->sp_.unhandled_exception(error);
			  }
            });
        }
        else {
          this->owner_->incoming_->write_async(this->sp_, this->buffer_, (size_t)dwBytesTransfered)
            .execute(
              [pthis](const std::shared_ptr<std::exception> & error) {
				  if (!error) {
					  static_cast<_read_socket_task *>(pthis.get())->read_async();
				  }
				  else {
					  static_cast<_read_socket_task *>(pthis.get())->sp_.unhandled_exception(error);
				  }
              });
        }
      }
      void error(DWORD error_code) override
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv error(%d)", error_code);
        if (ERROR_NETNAME_DELETED == error_code) {
          this->process(0);
        }
        else {
          this->sp_.unhandled_exception(std::make_shared<std::system_error>(error_code, std::system_category(), "read failed"));
        }
      }
    };

    class _write_socket_task : public _socket_task
    {
    public:
      constexpr static size_t BUFFER_SIZE = 1024;

      _write_socket_task(
        const service_provider & sp,
        const std::shared_ptr<_tcp_network_socket> & owner)
        : owner_(owner),
          sp_(sp)
      {
      }

      ~_write_socket_task()
      {
      }

      void write_async()
      {
        auto pthis = this->shared_from_this();
        this->owner_->outgoing_->read_async(this->sp_, this->buffer_, BUFFER_SIZE)
          .execute([pthis](const std::shared_ptr<std::exception> & ex, size_t len) {
			if(!ex){
              static_cast<_write_socket_task *>(pthis.get())->schedule(static_cast<_write_socket_task *>(pthis.get())->buffer_, len);
			}
			else {
				static_cast<_write_socket_task *>(pthis.get())->sp_.unhandled_exception(ex);
			}
            });
      }

      void check_timeout(const service_provider & sp) override
      {
      }

      void prepare_to_stop(const service_provider & sp) override
      {
      }

    private:
      service_provider sp_;
      std::shared_ptr<_tcp_network_socket> owner_;
      std::shared_ptr<_socket_task> pthis_;
      uint8_t buffer_[BUFFER_SIZE];

      void schedule(const void * data, size_t len)
      {
        if (0 == len) {
          shutdown(this->owner_->s_, SD_SEND);
          this->owner_.reset();
          return;
        }

        this->wsa_buf_.buf = (CHAR *)data;
        this->wsa_buf_.len = len;

        this->pthis_ = this->shared_from_this();

        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend");
        if (NOERROR != WSASend(this->owner_->s_, &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend error(%d)", errorCode);
            throw std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
      }

      void process(DWORD dwBytesTransfered) override
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend got(%d)", dwBytesTransfered);

        auto pthis = this->pthis_;
        this->pthis_.reset();

        try {
          if (this->wsa_buf_.len == dwBytesTransfered) {
            this->write_async();
          }
          else {
            this->schedule(this->wsa_buf_.buf + dwBytesTransfered, this->wsa_buf_.len - dwBytesTransfered);
          }
        }
        catch (const std::exception & ex) {
          this->sp_.unhandled_exception(std::make_shared<std::exception>(ex));
        }
        catch (...) {
          this->sp_.unhandled_exception(std::make_shared<std::runtime_error>("Unexpected error"));
        }
      }

      void error(DWORD error_code) override
      {
        this->sp_.unhandled_exception(std::make_shared<std::system_error>(error_code, std::system_category(), "write failed"));
      }
    };


#else
    class _socket_handler : public _socket_task
    {
      using this_class = _socket_handler;
    public:
      _socket_handler(
        const service_provider & sp,
        SOCKET_HANDLE s)
        : sp_(sp), s_(s),
          network_service_(static_cast<_network_service *>(sp.get<inetwork_service>())),
          event_masks_(EPOLLIN | EPOLLET),
          read_timeout_ticks_(0),
          read_status_(read_status_t::bof),
          read_result_([](const std::shared_ptr<std::exception> & , size_t){
            throw std::runtime_error("Logic error");
          }),
          write_result_([](const std::shared_ptr<std::exception> & ){
            throw std::runtime_error("Logic error");
          })
      {
      }
      
      ~_socket_handler()
      {
        if(EPOLLET != this->event_masks_){
          this->network_service_->remove_association(this->sp_, this->s_);
        }
      }

      void start()
      {
        this->network_service_->associate(
            this->sp_,
            this->s_,
            this->shared_from_this(),
            this->event_masks_);
      }

      async_task<size_t /*readed*/> read_async(uint8_t * data, size_t size) {

        std::lock_guard<std::mutex> lock(this->read_mutex_);
        if(read_status_t::waiting_data == this->read_status_){
          throw std::runtime_error("Invalid operation");
        }

        int len = read(this->s_, data, size);

        if (len < 0) {
          int error = errno;
          if(EAGAIN == error){
            this->read_buffer_ = data;
            this->read_buffer_size_ = size;
            this->read_status_ = read_status_t::waiting_data;

            return [pthis = this->shared_from_this()](
                const async_result<size_t /*readed*/> & result){
              auto this_ = static_cast<_socket_handler *>(pthis.get());
              this_->read_result_ = result;
              this_->change_mask(EPOLLIN);
            };
          }

          return async_task<size_t>(
              std::make_shared<std::system_error>(
                  error, std::system_category(), "recv"));
        }

        if(0 == len){
          this->read_status_ = read_status_t::eof;
        }

        this->read_timeout_ticks_ = 0;
        this->sp_.get<logger>()->trace("TCP", this->sp_, "got %d bytes", len);
        return async_task<size_t>(len);
      }

      async_task<> write_async(const uint8_t * data, size_t size) {
        std::lock_guard<std::mutex> lock(this->write_mutex_);
        switch (this->write_status_){
          case write_status_t::bof:
            this->write_buffer_ = data;
            this->write_buffer_size_ = size;
            return [pthis = this->shared_from_this()](const async_result<> & result){
              auto this_ = static_cast<_socket_handler *>(pthis.get());
              this_->write_result_ = result;
              this_->write_status_ = write_status_t::waiting_socket;
            };

          case write_status_t::eof:
          case write_status_t::waiting_socket:
            throw  std::runtime_error("Invalid operator");
        }

      }

      void prepare_to_stop(const service_provider & sp) override
      {
        this->change_mask(0, EPOLLIN);

        this->sp_.get<logger>()->trace("TCP", this->sp_, "read timeout");

        std::lock_guard<std::mutex> lock(this->read_mutex_);
        switch(this->read_status_) {
          case read_status_t::waiting_data:
            this->read_status_ = read_status_t::eof;
            this->read_result_.done(0);
            break;
          case read_status_t::bof:
            this->read_status_ = read_status_t::eof;
            break;
        }
      }
 
      void process(uint32_t events) override
      {
        if(EPOLLOUT == (EPOLLOUT & events)){
          this->change_mask(0, EPOLLOUT);

          std::lock_guard<std::mutex> lock(this->write_mutex_);

          if(write_status_t::waiting_socket != this->write_status_){
            throw std::runtime_error("Invalid design");
          }
          
          int len = write(
            this->s_,
            this->write_buffer_,
            this->write_buffer_size_);
          
          if (len < 0) {
            int error = errno;
            this->write_result_.error(
                std::make_shared<std::system_error>(
                  error,
                  std::generic_category(),
                  "Send"));
          } else {

            if ((size_t) len != this->write_buffer_size_) {
              throw std::runtime_error("Invalid send TCP");
            }

            this->sp_.get<logger>()->trace("TCP", this->sp_, "Sent %d bytes", len);
          }
        }
        
        if(EPOLLIN == (EPOLLIN & events)){
          this->change_mask(0, EPOLLIN);

          std::lock_guard<std::mutex> lock(this->read_mutex_);

          if(read_status_t::waiting_data != this->read_status_){
            throw std::runtime_error("Invalid design");
          }

          int len = read(
              this->s_,
              this->read_buffer_,
              this->read_buffer_size_);

          if (len < 0) {
            int error = errno;
            this->read_result_.error(
                std::make_shared<std::system_error>(
                    error,
                    std::generic_category(),
                    "Read"));
          } else {
            this->sp_.get<logger>()->trace("TCP", this->sp_, "Read %d bytes", len);
            this->read_result_.done(len);
          }
        }
      }
      
      void check_timeout(const service_provider & sp) override
      {
        std::lock_guard<std::mutex> lock(this->read_mutex_);

        sp.get<logger>()->trace("TCP", sp, "check_timeout(ticks=%d, is_closed=%s)",
           this->read_timeout_ticks_,
           ((read_status_t::eof == this->read_status_) ? "true" : "false"));
        
        if(1 < this->read_timeout_ticks_++ && (read_status_t::eof != this->read_status_)){
          this->change_mask(0, EPOLLIN | EPOLLOUT);

          if(read_status_t::waiting_data != this->read_status_){
            this->read_result_.done(0);
          }

          this->sp_.get<logger>()->trace("TCP", this->sp_, "read timeout");
          this->read_status_ = read_status_t::eof;
        }
      }

    private:
      SOCKET_HANDLE s_;
      service_provider sp_;
      _network_service * network_service_;

      enum class read_status_t{
        bof,
        waiting_data,
        eof
      };

      std::mutex read_mutex_;
      read_status_t read_status_;
      async_result<size_t> read_result_;
      uint8_t * read_buffer_;
      size_t read_buffer_size_;

      enum class write_status_t {
        bof,
        waiting_socket,
        eof
      };

      std::mutex write_mutex_;
      write_status_t write_status_;
      async_result<> write_result_;
      const uint8_t * write_buffer_;
      size_t write_buffer_size_;

      std::mutex event_masks_mutex_;
      uint32_t event_masks_;
      int read_timeout_ticks_;

      void change_mask(uint32_t set_events, uint32_t clear_events = 0)
      {
        std::unique_lock<std::mutex> lock(this->event_masks_mutex_);
        auto need_create = (EPOLLET == this->event_masks_);
        this->event_masks_ |= set_events;
        this->event_masks_ &= ~clear_events;
        
        if(!need_create && EPOLLET != this->event_masks_){
          this->network_service_->set_events(this->sp_, this->s_, this->event_masks_);
        }
        else if (EPOLLET == this->event_masks_){
          this->network_service_->remove_association(this->sp_, this->s_);
        }
        else {
          this->network_service_->associate(this->sp_, this->s_, this->shared_from_this(), this->event_masks_);
        }
      }
    };
#endif//_WIN32
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_P_H_
