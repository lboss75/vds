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

  class _tcp_network_socket
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

    static std::shared_ptr<tcp_network_socket> from_handle(SOCKET_HANDLE handle)
    {
      return std::shared_ptr<tcp_network_socket>(new tcp_network_socket(new _tcp_network_socket(handle)));
    }
    //
    //    std::shared_ptr<vds::stream_input_async<uint8_t>> start(const service_provider & sp)
    //    {
    //      sp.get<logger>()->trace("TCP", sp, "socket start");
    //      
    //#ifdef _WIN32
    //      this->socket_task_ = std::make_shared<_write_socket_task>(sp, this->s_);
    //      return std::make_shared<_read_socket_task>(sp, this->shared_from_this());
    //#else
    //      auto handler = std::make_shared<_socket_handler>(sp, this->shared_from_this());
    //      this->socket_task_ = handler;
    //      return handler->start();
    //#endif//_WIN32
    //    }
    //
    //    std::future<void> write_async(const uint8_t * data, size_t size) {
    //#ifdef _WIN32
    //      return static_cast<_write_socket_task *>(this->socket_task_.get())->write_async(data, size);
    //#else
    //      auto task = this->socket_task_;
    //      return static_cast<_socket_handler *>(task.get())->write_async(data, size);
    //#endif//_WIN32
    //    }

#ifndef _WIN32
    void make_socket_non_blocking()
    {
      auto flags = fcntl(this->handle(), F_GETFL, 0);
      if (flags == -1) {
        throw std::runtime_error("fcntl");
      }

      flags |= O_NONBLOCK;
      auto s = fcntl(this->handle(), F_SETFL, flags);
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
  };

#ifdef _WIN32
  class _read_socket_task : public _socket_task, public stream_input_async<uint8_t>
  {
  public:
    constexpr static size_t BUFFER_SIZE = 1024;

    _read_socket_task(
      const std::shared_ptr<tcp_network_socket> & owner)
      : owner_(owner) {
    }

    ~_read_socket_task() {
    }


    std::future<size_t> read_async(
      const service_provider &sp,
      uint8_t * buffer,
      size_t len) override {
      memset(&this->overlapped_, 0, sizeof(this->overlapped_));
      this->wsa_buf_.len = len;
      this->wsa_buf_.buf = (CHAR *)buffer;
      this->pthis_ = this->shared_from_this();
      auto r = std::make_shared<std::promise<size_t>>();
      this->result_ = r;

      sp.get<logger>()->trace("TCP", sp, "WSARecv");
      DWORD flags = 0;
      DWORD numberOfBytesRecvd;
      if (NOERROR != WSARecv((*this->owner_)->handle(),
        &this->wsa_buf_,
        1,
        &numberOfBytesRecvd,
        &flags,
        &this->overlapped_,
        NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
          sp.get<logger>()->trace("TCP", sp, "WSARecv error");
          auto pthis = this->pthis_;
          this->pthis_.reset();

          if (WSAESHUTDOWN == errorCode || WSAECONNABORTED == errorCode) {
            auto t = std::move(this->result_);
            t->set_value(0);
            return t->get_future();
          }
          else {
            auto t = std::move(this->result_);
            t->set_exception(
              std::make_exception_ptr(std::system_error(errorCode, std::system_category(), "read from tcp socket")));
            return t->get_future();
          }
        }
      }

      return r->get_future();
    }


  private:
    std::shared_ptr<tcp_network_socket> owner_;
    std::shared_ptr<stream_input_async<uint8_t>> pthis_;
    std::shared_ptr<std::promise<size_t>> result_;

    void process(DWORD dwBytesTransfered) override
    {
      auto r = std::move(this->result_);
      auto pthis = std::move(this->pthis_);

      if (0 == dwBytesTransfered) {
        r->set_value(0);
      }
      else {
        r->set_value((size_t)dwBytesTransfered);
      }
    }

    void error(DWORD error_code) override
    {
      if (ERROR_NETNAME_DELETED == error_code || WSAECONNABORTED == error_code) {
        this->process(0);
      }
      else {
        auto pthis = std::move(this->pthis_);
        auto r = std::move(this->result_);
        r->set_exception(std::make_exception_ptr(std::system_error(error_code, std::system_category(), "read failed")));
      }
    }
  };

  class _write_socket_task : public _socket_task, public stream_output_async<uint8_t>
  {
  public:
    constexpr static size_t BUFFER_SIZE = 1024;

    _write_socket_task(const std::shared_ptr<tcp_network_socket> & owner)
      : owner_(owner)
    {
    }

    ~_write_socket_task()
    {
    }

    std::future<void> write_async(const service_provider &/*sp*/, const uint8_t * data, size_t len) override
    {
      this->result_ = std::make_shared<std::promise<void>>();
      this->pthis_ = this->shared_from_this();
      this->schedule(data, len);

      return this->result_->get_future();
    }

  private:
    std::shared_ptr<tcp_network_socket> owner_;
    std::shared_ptr<std::promise<void>> result_;
    std::shared_ptr<stream_output_async<uint8_t>> pthis_;

    void schedule(const void * data, size_t len)
    {
      if (0 == len) {
        shutdown((*this->owner_)->handle(), SD_SEND);
        this->result_->set_value();
      }
      else {
        memset(&this->overlapped_, 0, sizeof(this->overlapped_));
        this->wsa_buf_.buf = (CHAR *)data;
        this->wsa_buf_.len = (ULONG)len;

        if (NOERROR != WSASend((*this->owner_)->handle(), &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
      }
    }


    void process(DWORD dwBytesTransfered) override
    {
      auto pthis = this->pthis_;
      this->pthis_.reset();

      try {
        if (this->wsa_buf_.len == dwBytesTransfered) {
          this->result_->set_value();
        }
        else {
          this->schedule(this->wsa_buf_.buf + dwBytesTransfered, this->wsa_buf_.len - dwBytesTransfered);
        }
      }
      catch (...) {
        this->result_->set_exception(std::current_exception());
      }
    }

    void error(DWORD error_code) override
    {
      this->result_->set_exception(std::make_exception_ptr(std::system_error(error_code, std::system_category(), "write failed")));
    }
  };


#else
  class _socket_handler : public _socket_task_impl<_socket_handler> {
    using base_class = _socket_task_impl<_socket_handler>;

  public:
    _socket_handler(
      const service_provider &sp,
      const std::shared_ptr<_stream_async<uint8_t>> &owner)
      : _socket_task_impl<_socket_handler>(sp, static_cast<_tcp_network_socket *>(owner.get())->s_),
      owner_(owner) {
    }

    ~_socket_handler() {
    }

    std::shared_ptr<vds::stream_input_async<uint8_t>> start() {
      base_class::start();
      this->reader_ = std::make_shared<socket_reader>(this);
      this->change_mask(EPOLLIN);
      return this->reader_;
    }


    std::future<void> write_async(const uint8_t *data, size_t size) {
      std::lock_guard<std::mutex> lock(this->write_mutex_);
      switch (this->write_status_) {
      case write_status_t::bof: {
        this->write_buffer_.resize(size);
        memcpy(this->write_buffer_.data(), data, size);

        this->write_result_ = std::make_shared<std::promise<void>>();
        this->write_status_ = write_status_t::waiting_socket;
        this->change_mask(EPOLLOUT);
        return this->write_result_->get_future();
      }

      case write_status_t::continue_write: {
        this->write_buffer_.resize(size);
        memcpy(this->write_buffer_.data(), data, size);
        this->write_result_ = std::make_shared<std::promise<void>>();
        this->write_data();
        return this->write_result_->get_future();
      }

      case write_status_t::eof:
        throw std::system_error(ECONNRESET, std::system_category());

      default:
        throw std::runtime_error("Invalid operator");
      }
    }

    void write_data() {
      std::unique_lock<std::mutex> lock(this->write_mutex_);

      if (0 == this->write_buffer_.size()) {
        shutdown(this->s_, SHUT_WR);
        this->write_status_ = write_status_t::eof;
        return;
      }

      int len = send(
        this->s_,
        this->write_buffer_.data(),
        this->write_buffer_.size(),
        MSG_NOSIGNAL);

      if (len < 0) {
        int error = errno;
        if (EAGAIN == error) {
          this->write_status_ = write_status_t::waiting_socket;
          this->change_mask(EPOLLOUT);
          return;
        }

        this->write_status_ = write_status_t::waiting_socket;
        auto result = std::move(this->write_result_);
        lock.unlock();

        result->set_exception(std::make_exception_ptr(
          std::system_error(
            error,
            std::generic_category(),
            "Send")));
      }
      else {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "Sent %d bytes", len);

        if ((size_t)len != this->write_buffer_.size()) {
          throw std::runtime_error("Invalid send TCP");
        }
        this->write_status_ = write_status_t::continue_write;
        auto result = std::move(this->write_result_);
        lock.unlock();

        result->set_value();
      }
    }

    void read_data() {
      this->reader_->continue_read();
    }

  private:
    std::shared_ptr<_stream_async<uint8_t>> owner_;

    const_data_buffer write_buffer_;
    std::shared_ptr<std::promise<void>> write_result_;


    class socket_reader : public stream_input_async<uint8_t> {
    public:
      socket_reader(_socket_handler * owner)
        : owner_(owner) {
      }

      std::future<size_t> read_async(
        const service_provider &sp,
        uint8_t * buffer,
        size_t buffer_size) override {
        int len = read(
          this->owner_->s_,
          buffer,
          buffer_size);

        if (len <= 0) {
          int error = errno;
          if (EAGAIN == error) {
            this->owner_->read_status_ = read_status_t::waiting_socket;
            this->owner_->change_mask(EPOLLIN);
            this->buffer_ = buffer;
            this->buffer_size_ = buffer_size;
            this->result_ = std::make_shared<std::promise<size_t>>();
            co_return co_await this->result_->get_future();
          }
          if (0 == error && 0 == len) {
            co_return 0;
          }

          sp.get<logger>()->trace("TCP", sp, "Read error %d", error);
          throw std::system_error(
            error,
            std::generic_category(),
            "Read");
        }
        else {
          sp.get<logger>()->trace("TCP", sp, "Read %d bytes", len);
          co_return len;
        }
      }

      void continue_read() {
        int len = read(
          this->owner_->s_,
          this->buffer_,
          this->buffer_size_);

        if (len <= 0) {
          int error = errno;
          if (EAGAIN == error) {
            this->owner_->read_status_ = read_status_t::waiting_socket;
            this->owner_->change_mask(EPOLLIN);
            return;
          }
          if (0 == error && 0 == len) {
            this->result_->set_value(0);
            this->result_.reset();
            return;
          }

          this->result_->set_exception(std::make_exception_ptr(
            std::system_error(
              error,
              std::generic_category(),
              "Read")));
        }
        else {
          this->result_->set_value(len);
        }
      }


    private:
      _socket_handler * owner_;
      std::shared_ptr<std::promise<size_t>> result_;
      uint8_t * buffer_;
      size_t buffer_size_;
    };

    std::shared_ptr<socket_reader> reader_;
  };

#endif//_WIN32
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_P_H_
