#ifndef __VDS_NETWORK_NETWORK_SOCKET_P_H_
#define __VDS_NETWORK_NETWORK_SOCKET_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_types_p.h"
#include "tcp_network_socket.h"
#include "socket_task_p.h"
#include "private/network_service_p.h"

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

    void start(const service_provider & sp, const stream<uint8_t> & target)
    {
      sp.get<logger>()->trace("TCP", sp, "socket start");
      
#ifdef _WIN32
      std::make_shared<_read_socket_task>(sp, this->shared_from_this(), target)->start();
      this->socket_task_ = std::make_shared<_write_socket_task>(sp, this->s_);
#else
      auto handler = std::make_shared<_socket_handler>(sp, this->shared_from_this(), target);
      this->socket_task_ = handler;
      handler->start();
#endif//_WIN32
    }

    async_task<> write_async(const uint8_t * data, size_t size) override {
#ifdef _WIN32
      return static_cast<_write_socket_task *>(this->socket_task_.get())->write_async(data, size);
#else
      auto task = this->socket_task_;
      return static_cast<_socket_handler *>(task.get())->write_async(data, size);
#endif//_WIN32
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
    std::shared_ptr<_socket_task> socket_task_;
#else
    std::shared_ptr<_socket_task> socket_task_;
#endif//_WIN32

#ifdef _WIN32
    class _read_socket_task : public _socket_task
    {
    public:
      constexpr static size_t BUFFER_SIZE = 1024;

      _read_socket_task(
        const service_provider & sp,
        const std::shared_ptr<_stream_async<uint8_t>> & owner,
        const stream<uint8_t> & target)
        : owner_(owner),
          sp_(sp),
          target_(target)
      {
      }

      ~_read_socket_task()
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv closed");
      }

      void start()
      {
        this->wsa_buf_.len = BUFFER_SIZE;
        this->wsa_buf_.buf = (CHAR *)this->buffer_;

        this->pthis_ = this->shared_from_this();

        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv");
        DWORD flags = 0;
        DWORD numberOfBytesRecvd;
        if (NOERROR != WSARecv(static_cast<_tcp_network_socket *>(this->owner_.get())->s_,
          &this->wsa_buf_,
          1,
          &numberOfBytesRecvd,
          &flags,
          &this->overlapped_,
          NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv error");
            auto pthis = this->pthis_;
            this->pthis_.reset();

            if (WSAESHUTDOWN == errorCode || WSAECONNABORTED == errorCode) {
              this->target_.write(nullptr, 0);
            }
            else {
              this->sp_.unhandled_exception(std::make_unique<std::system_error>(errorCode, std::system_category(), "read from tcp socket"));
            }
            return;
          }
        }
      }


    private:
      service_provider sp_;
      std::shared_ptr<_stream_async<uint8_t>> owner_;
      stream<uint8_t> target_;
      uint8_t buffer_[BUFFER_SIZE];
      std::shared_ptr<_socket_task> pthis_;

      void process(DWORD dwBytesTransfered) override
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv got(%d)", dwBytesTransfered);
        auto pthis = this->pthis_;
        this->pthis_.reset();

        if (0 == dwBytesTransfered) {
          this->target_.write(nullptr, 0);
          this->start();
        }
        else {
          this->target_.write(this->buffer_, (size_t)dwBytesTransfered);
          this->start();
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
        SOCKET_HANDLE s)
        : sp_(sp),
          s_(s),
        result_([](const std::shared_ptr<std::exception> &) {
          throw std::runtime_error("Logic error");
        })
      {
      }

      ~_write_socket_task()
      {
      }

      async_task<> write_async(const uint8_t * data, size_t len)
      {
        return [pthis = this->shared_from_this(), data, len](const async_result<> & result){
          auto this_ = static_cast<_write_socket_task *>(pthis.get());
          
          this_->result_ = result;
          this_->pthis_ = pthis;
          auto ex = this_->schedule(data, len);
          if (ex) {
            result.error(ex);
          }
        };
      }

    private:
      service_provider sp_;
      SOCKET_HANDLE s_;
      async_result<> result_;
      std::shared_ptr<_socket_task> pthis_;

      std::shared_ptr<std::exception> schedule(const void * data, size_t len)
      {
        if (0 == len) {
          shutdown(this->s_, SD_SEND);
        }
        else {

          this->wsa_buf_.buf = (CHAR *)data;
          this->wsa_buf_.len = (ULONG)len;

          this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend");
          if (NOERROR != WSASend(this->s_, &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
            auto errorCode = WSAGetLastError();
            if (WSA_IO_PENDING != errorCode) {
              this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend error(%d)", errorCode);
              return std::make_shared<std::system_error>(errorCode, std::system_category(), "WSASend failed");
            }
          }
        }

        return std::shared_ptr<std::exception>();
      }
      
      
      void process(DWORD dwBytesTransfered) override
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend got(%d)", dwBytesTransfered);

        auto pthis = this->pthis_;
        this->pthis_.reset();

        try {
          if (this->wsa_buf_.len == dwBytesTransfered) {
            this->result_.done();
          }
          else {
            auto ex = this->schedule(this->wsa_buf_.buf + dwBytesTransfered, this->wsa_buf_.len - dwBytesTransfered);
            if (ex) {
              this->sp_.unhandled_exception(ex);
            }
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
    class _socket_handler : public _socket_task_impl<_socket_handler>
    {
      using base_class = _socket_task_impl<_socket_handler>;

    public:
      _socket_handler(
        const service_provider & sp,
        const std::shared_ptr<_stream_async<uint8_t>> & owner,
        const stream<uint8_t> & target)
        : _socket_task_impl<_socket_handler>(sp, static_cast<_tcp_network_socket *>(owner.get())->s_),
          owner_(owner),
          target_(target)
      {
      }

      ~_socket_handler(){
        std::cout << "_socket_handler::~_socket_handler";
      }

      void start(){
        base_class::start();
        this->change_mask(EPOLLIN);
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
              this_->change_mask(EPOLLOUT);
            };

          case write_status_t::eof:
          case write_status_t::waiting_socket:
          default:
            throw  std::runtime_error("Invalid operator");
        }

      }

      void write_data() {
        std::unique_lock<std::mutex> lock(this->write_mutex_);

        if(write_status_t::waiting_socket != this->write_status_) {
          throw std::runtime_error("Invalid operation");
        }

        int len = write(
            this->s_,
            this->write_buffer_,
            this->write_buffer_size_);

        if (len < 0) {
          int error = errno;
          this->write_status_ = write_status_t::bof;
          lock.unlock();

          this->write_result_.error(
              std::make_shared<std::system_error>(
                  error,
                  std::generic_category(),
                  "Send"));
        } else {
          this->write_status_ = write_status_t::bof;
          this->sp_.get<logger>()->trace("TCP", this->sp_, "Sent %d bytes", len);

          if ((size_t) len != this->write_buffer_size_) {
            throw std::runtime_error("Invalid send TCP");
          }
          lock.unlock();

          this->write_result_.done();
        }
      }

      void read_data() {
        for(;;) {
          int len = read(
              this->s_,
              this->read_buffer_,
              sizeof(this->read_buffer_));

          if (len < 0) {
            int error = errno;
            if (EAGAIN == error) {
              break;
            }

            throw std::system_error(
                    error,
                    std::generic_category(),
                    "Read");
          } else {
            this->sp_.get<logger>()->trace("TCP", this->sp_, "Read %d bytes", len);
            this->target_.write(this->read_buffer_, len);
          }
        }
      }

    private:
      std::shared_ptr<_stream_async<uint8_t>> owner_;

      const uint8_t * write_buffer_;
      size_t write_buffer_size_;
      async_result<> write_result_;

      stream<uint8_t> target_;
      uint8_t read_buffer_[1024];
    };
#endif//_WIN32
  };

  inline _tcp_network_socket * tcp_network_socket::operator ->() const {
    return static_cast<_tcp_network_socket *>(this->impl_.get());
  }

}

#endif//__VDS_NETWORK_NETWORK_SOCKET_P_H_
