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
#include "logger.h"

namespace vds {
  class _network_service;

  class _tcp_network_socket
  {
  public:
//    _tcp_network_socket()
//      : s_(INVALID_SOCKET)
//#ifndef _WIN32
//    , event_masks_(EPOLLET)
//#endif
//    {
//    }

    _tcp_network_socket(
#ifndef _WIN32
      const service_provider * sp,
#endif
        SOCKET_HANDLE s)
      : s_(s)
#ifndef _WIN32
        , sp_(sp), event_masks_(0)
#endif
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

    bool operator ! () const {
#ifdef _WIN32
        return  (INVALID_SOCKET == this->s_);
#else
        return (0 >= this->s_);
#endif
    }

    static std::shared_ptr<tcp_network_socket> from_handle(
#ifndef _WIN32
      const service_provider * sp,
#endif
      SOCKET_HANDLE handle)
    {
      return std::shared_ptr<tcp_network_socket>(
          new tcp_network_socket(
              new _tcp_network_socket(
#ifndef _WIN32
                sp,
#endif
                handle)));
    }
    //
    //    std::shared_ptr<vds::stream_input_async<uint8_t>> start()
    //    {
    //      sp->get<logger>()->trace("TCP", sp, "socket start");
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
    //    vds::async_task<void> write_async(const uint8_t * data, size_t size) {
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

    void process(uint32_t events);

    void change_mask(
        const std::shared_ptr<socket_base> & s,
        uint32_t set_events,
        uint32_t clear_events = 0)
    {
      std::unique_lock<std::mutex> lock(this->event_masks_mutex_);
      auto last_mask = this->event_masks_;
      this->event_masks_ |= set_events;
      this->event_masks_ &= ~clear_events;
      if(last_mask == this->event_masks_){
        return;
      }

      if(0 != last_mask && 0 != this->event_masks_){
        (*this->sp_->get<network_service>())->set_events(this->s_, this->event_masks_);
      }
      else if (0 == this->event_masks_){
        (*this->sp_->get<network_service>())->remove_association(this->s_);
      }
      else {
        (*this->sp_->get<network_service>())->associate(this->s_, s, this->event_masks_);
      }
    }

#endif//_WIN32

  private:
    friend class tcp_network_socket;

    SOCKET_HANDLE s_;

#ifndef _WIN32
    const service_provider * sp_;
    std::mutex event_masks_mutex_;
    uint32_t event_masks_;

    std::weak_ptr<class _read_socket_task> read_task_;
    std::weak_ptr<class _write_socket_task> write_task_;

#endif
  };


#ifdef _WIN32
  class _read_socket_task : public _socket_task, public stream_input_async<uint8_t>
  {
  public:
    constexpr static size_t BUFFER_SIZE = 1024;

    _read_socket_task(
      const service_provider * sp,
      const std::shared_ptr<tcp_network_socket> & owner)
      : sp_(sp), owner_(owner) {
    }

    ~_read_socket_task() {
    }


    vds::async_task<size_t> read_async(      
      uint8_t * buffer,
      size_t len) override {
      vds_assert(!this->result_);
      memset(&this->overlapped_, 0, sizeof(this->overlapped_));
      this->wsa_buf_.len = len;
      this->wsa_buf_.buf = (CHAR *)buffer;
      this->pthis_ = this->shared_from_this();
      auto r = std::make_shared<vds::async_result<size_t>>();
      this->result_ = r;

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
          this->sp_->get<logger>()->trace("TCP", "WSARecv error");
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
    const service_provider * sp_;
    std::shared_ptr<tcp_network_socket> owner_;
    std::shared_ptr<stream_input_async<uint8_t>> pthis_;
    std::shared_ptr<vds::async_result<size_t>> result_;

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

    _write_socket_task(
      const service_provider * sp,
      const std::shared_ptr<tcp_network_socket> & owner)
      : sp_(sp), owner_(owner)
    {
    }

    ~_write_socket_task()
    {
    }

    vds::async_task<void> write_async(const uint8_t * data, size_t len) override
    {
      if (0 == len) {
        shutdown((*this->owner_)->handle(), SD_SEND);
        co_return;
      }

      auto r = std::make_shared<vds::async_result<void>>();
      this->result_ = r;
      this->pthis_ = this->shared_from_this();
      this->schedule(data, len);

      co_return co_await r->get_future();
    }

  private:
    const service_provider * sp_;
    std::shared_ptr<tcp_network_socket> owner_;
    std::shared_ptr<vds::async_result<void>> result_;
    std::shared_ptr<stream_output_async<uint8_t>> pthis_;

    void schedule(const void * data, size_t len)
    {
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


    void process(DWORD dwBytesTransfered) override
    {
      auto pthis = this->pthis_;
      this->pthis_.reset();

      if (this->wsa_buf_.len == dwBytesTransfered) {
        auto r = std::move(this->result_);
        r->set_value();
      }
      else {
        this->schedule(this->wsa_buf_.buf + dwBytesTransfered, this->wsa_buf_.len - dwBytesTransfered);
      }
    }

    void error(DWORD error_code) override
    {
      auto r = std::move(this->result_);
      r->set_exception(std::make_exception_ptr(std::system_error(error_code, std::system_category(), "write failed")));
    }
  };


#else
  class _write_socket_task : public stream_output_async<uint8_t> {

  public:
    _write_socket_task(
      const std::shared_ptr<socket_base> &owner)
      : owner_(owner) {
    }

    ~_write_socket_task() {
    }

    vds::async_task<void> write_async(
        const uint8_t *data,
        size_t size) override {

      auto r = std::make_shared<vds::async_result<void>>();
      if(0 == size){
        shutdown((*this->owner())->handle(), SHUT_WR);
        r->set_value();
      }
      else {
        for (;;) {
          int len = send(
              (*this->owner())->handle(),
              data,
              size,
              MSG_NOSIGNAL);

          if (len < 0) {
            int error = errno;
            if (EAGAIN == error) {
              this->buffer_ = data;
              this->buffer_size_ = size;
              this->result_ = r;
              (*this->owner())->change_mask(this->owner_, EPOLLOUT);
            } else {
              r->set_exception(std::make_exception_ptr(
                  std::system_error(error, std::generic_category(), "Send TCP")));
            }
          } else {
            if ((size_t) len < size) {
              data += len;
              size -= len;

              continue;
            }

            r->set_value();
          }

          break;
        }
      }

      return r->get_future();
    }

    void process() {
      for (;;) {
        int len = send(
            (*this->owner())->handle(),
            this->buffer_,
            this->buffer_size_,
            MSG_NOSIGNAL);

        if (len < 0) {
          int error = errno;
          if (EAGAIN == error) {
            return;
          }

          (*this->owner())->change_mask(this->owner_, 0, EPOLLOUT);
          auto r = std::move(this->result_);
          r->set_exception(std::make_exception_ptr(
              std::system_error(
                  error,
                  std::generic_category(),
                  "Send")));
        } else {
          if ((size_t) len < this->buffer_size_) {
            this->buffer_ += len;
            this->buffer_size_ -= len;
            continue;
          }

          (*this->owner())->change_mask(this->owner_, 0, EPOLLOUT);
          auto r = std::move(this->result_);
          r->set_value();
        }

        break;
      }
    }

  private:
    std::shared_ptr<socket_base> owner_;
    std::shared_ptr<vds::async_result<void>> result_;
    const uint8_t * buffer_;
    size_t buffer_size_;

    tcp_network_socket * owner() const {
      return static_cast<tcp_network_socket *>(this->owner_.get());
    }
  };

  class _read_socket_task : public stream_input_async<uint8_t>
  {
  public:
    _read_socket_task(
        const service_provider * sp,
        const std::shared_ptr<socket_base> &owner)
        : sp_(sp),
          owner_(owner) {
    }

    ~_read_socket_task() {
    }

    vds::async_task<size_t> read_async(
        uint8_t * buffer,
        size_t buffer_size) override {
      auto r = std::make_shared<vds::async_result<size_t>>();
      if(!(*this->owner())){
          r->set_value(0);
          return r->get_future();
      }

      int len = read(
          (*this->owner())->handle(),
          buffer,
          buffer_size);

      if (len <= 0) {
        int error = errno;
        if (EAGAIN == error) {
          this->buffer_ = buffer;
          this->buffer_size_ = buffer_size;
          this->result_ = r;
          (*this->owner())->change_mask(this->owner_, EPOLLIN);
        }
        else if ((0 == error || EINTR == error || ENOENT == error) && 0 == len) {
          r->set_value(0);
        }
        else {
          this->sp_->get<logger>()->trace("TCP", "Read error %d", error);
          r->set_exception(std::make_exception_ptr(
              std::system_error(
                  error,
                  std::generic_category(),
                  "Read")));
        }
      }
      else {
        this->sp_->get<logger>()->trace("TCP", "Read %d bytes", len);
        r->set_value(len);
      }

      return r->get_future();
    }

    void process() {
      int len = read(
          (*this->owner())->handle(),
          this->buffer_,
          this->buffer_size_);

      if (len <= 0) {
        int error = errno;
        if (EAGAIN == error) {
          return;
        }

        (*this->owner())->change_mask(this->owner_, 0, EPOLLIN);

        auto r = std::move(this->result_);
        if ((0 == error || EINTR == error || ENOENT == error) && 0 == len) {
          r->set_value(0);
        }
        else {
          r->set_exception(std::make_exception_ptr(
              std::system_error(
                  error,
                  std::generic_category(),
                  "Read")));
        }
      }
      else {
        (*this->owner())->change_mask(this->owner_, 0, EPOLLIN);

        auto r = std::move(this->result_);
        r->set_value(len);
      }
    }


  private:
    const service_provider * sp_;
    std::shared_ptr<socket_base> owner_;
    std::shared_ptr<vds::async_result<size_t>> result_;
    uint8_t * buffer_;
    size_t buffer_size_;

    tcp_network_socket * owner() const {
      return static_cast<tcp_network_socket *>(this->owner_.get());
    }
  };

#endif//_WIN32
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_P_H_
