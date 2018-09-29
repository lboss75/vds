#ifndef __VDS_NETWORK_UDP_SOCKET_P_H_
#define __VDS_NETWORK_UDP_SOCKET_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <udp_datagram_size_exception.h>
#include <vds_exceptions.h>
#include "network_types_p.h"
#include "service_provider.h"
#include "network_service_p.h"
#include "udp_socket.h"
#include "socket_task_p.h"
#include "const_data_buffer.h"
#include "vds_debug.h"

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


  class _udp_socket
  {
  public:
    _udp_socket(SOCKET_HANDLE s)
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

    void start(const vds::service_provider & sp)
    {
#ifdef _WIN32
#else
      auto handler = std::make_shared<_udp_handler>(sp, this->shared_from_this());
      this->handler_ = handler;
      handler->start();
#endif
    }

    void prepare_to_stop(const service_provider & sp)
    {
#ifdef _WIN32
#else
      this->handler_->prepare_to_stop(sp);
#endif// _WIN32
    }

    void stop()
    {
#ifdef _WIN32
#else
      this->handler_->stop();
      this->handler_.reset();
#endif
    }

    std::future<udp_datagram> read_async()
    {
#ifdef _WIN32
#else
      if (!this->handler_) {
        throw std::system_error(ECONNRESET, std::system_category(), "Socket is closed");
      }

      co_return co_await this->handler_->read_async();
#endif
    }

    std::future<void> write_async(const udp_datagram & message)
    {
#ifdef _WIN32
#else
      co_return co_await this->handler_->write_async(message);
#endif
    }


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
  };

#ifdef _WIN32
  class _udp_receive : public _socket_task, public udp_datagram_reader
  {
  public:
    _udp_receive(const std::shared_ptr<udp_socket> & s)
      : s_(s)
    {
    }

    std::future<udp_datagram> read_async(const service_provider & sp)
    {
      vds_assert(!this->result_);

      memset(&this->overlapped_, 0, sizeof(this->overlapped_));
      this->wsa_buf_.len = sizeof(this->buffer_);
      this->wsa_buf_.buf = (CHAR *)this->buffer_;

      auto r = std::make_shared<std::promise<udp_datagram>>();
      this->result_ = r;

      sp.get<logger>()->trace("UDP", sp, "WSARecvFrom %d", this->s_);

      DWORD flags = 0;
      DWORD numberOfBytesRecvd;
      if (NOERROR != WSARecvFrom(
        (*this->s_)->handle(),
        &this->wsa_buf_,
        1,
        &numberOfBytesRecvd,
        &flags,
        this->addr_,
        this->addr_.size_ptr(),
        &this->overlapped_,
        NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
          this->result_.reset();
          r->set_exception(
            std::make_exception_ptr(
              std::system_error(
                errorCode,
                std::system_category(),
                "WSARecvFrom failed")));
        }
        else {
          sp.get<logger>()->trace("UDP", sp, "Read scheduled");
        }
      }
      else {
        auto errorCode = WSAGetLastError();
        sp.get<logger>()->trace("UDP", sp, "Direct readed %d, code %d", numberOfBytesRecvd, errorCode);
        //this_->process(numberOfBytesRecvd);
      }

      return r->get_future();
    }

    void prepare_to_stop(const service_provider & sp)
    {
    }

  private:
    std::shared_ptr<udp_socket> s_;
    std::shared_ptr<std::promise<udp_datagram>> result_;

    network_address addr_;
    uint8_t buffer_[64 * 1024];

    void process(DWORD dwBytesTransfered) override
    {
      vds_assert(this->result_);
      auto pthis = this->shared_from_this();
      auto r = std::move(this->result_);
      r->set_value(_udp_datagram::create(this->addr_, this->buffer_, (size_t)dwBytesTransfered));
    }

    void error(DWORD error_code) override
    {
      auto r = std::move(this->result_);
      r->set_exception(
        std::make_exception_ptr(
          std::system_error(error_code, std::system_category(), "WSARecvFrom failed")));
    }
  };

  class _udp_send : public _socket_task, public udp_datagram_writer
  {
  public:
    _udp_send(const std::shared_ptr<udp_socket> & s)
      : s_(s) {
    }

    std::future<void> write_async(const service_provider & sp, const udp_datagram & data)
    {
      std::unique_lock<not_mutex> lock(this->not_mutex_);
      vds_assert(!this->result_);
      memset(&this->overlapped_, 0, sizeof(this->overlapped_));
      this->buffer_ = data;
      this->wsa_buf_.len = this->buffer_.data_size();
      this->wsa_buf_.buf = (CHAR *)this->buffer_.data();
      sp.get<logger>()->trace(
        "UDP",
        sp,
        "write_async %s %d bytes",
        this->buffer_->address().to_string().c_str(),
        this->buffer_.data_size());

      auto r = std::make_shared<std::promise<void>>();
      this->result_ = r;
      lock.unlock();


      sp.get<logger>()->trace(
        "UDP",
        sp,
        "WSASendTo %s %d bytes (%s)",
        this->buffer_->address().to_string().c_str(),
        this->buffer_.data_size(),
        base64::from_bytes(static_cast<const sockaddr *>(this->buffer_->address()), this->buffer_->address().size()).c_str()
      );
      if (NOERROR != WSASendTo(
        (*this->s_)->handle() ,
        &this->wsa_buf_,
        1,
        NULL,
        0,
        this->buffer_->address(),
        this->buffer_->address().size(),
        &this->overlapped_,
        NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
          r->set_exception(std::make_exception_ptr(std::system_error(errorCode, std::system_category(), "WSASend failed")));
          this->result_.reset();
        }
      }

      return r->get_future();
    }

    void prepare_to_stop(const service_provider & sp)
    {
    }

  private:
    std::shared_ptr<udp_socket> s_;
    std::shared_ptr<std::promise<void>> result_;

    socklen_t addr_len_;
    udp_datagram buffer_;
    not_mutex not_mutex_;

    void process(DWORD dwBytesTransfered) override
    {
      std::unique_lock<not_mutex> lock(this->not_mutex_);
      vds_assert(this->result_);
      auto result = std::move(this->result_);
      lock.unlock();

      if (this->wsa_buf_.len != (size_t)dwBytesTransfered) {
        result->set_exception(std::make_exception_ptr(std::runtime_error("Invalid sent UDP data")));
      }
      else {
        result->set_value();
      }
    }

    void error(DWORD error_code) override
    {
      std::unique_lock<not_mutex> lock(this->not_mutex_);
      vds_assert(this->result_);
      auto result = std::move(this->result_);
      lock.unlock();

      result->set_exception(std::make_exception_ptr(std::system_error(error_code, std::system_category(), "WSASendTo failed")));
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
      owner_(owner)
    {
    }

    ~_udp_handler()
    {
    }

    std::future<udp_datagram> read_async() {
      std::lock_guard<std::mutex> lock(this->read_mutex_);
      switch (this->read_status_) {
      case read_status_t::bof: {
        this->read_result_ = std::make_shared<std::promise<udp_datagram>>();
        this->read_status_ = read_status_t::waiting_socket;
        this->change_mask(EPOLLIN);
        return this->read_result_->get_future();
      }

      case read_status_t::continue_read: {
        this->read_result_ = std::make_shared<std::promise<udp_datagram>>();
        this->read_data();
        return this->read_result_->get_future();
      }

      case read_status_t::eof:
        throw std::system_error(ECONNRESET, std::system_category());

      default:
        throw  std::runtime_error("Invalid operator");
      }
    }

    std::future<void> write_async(const udp_datagram & message) {
      std::lock_guard<std::mutex> lock(this->write_mutex_);
      switch (this->write_status_) {
      case write_status_t::bof: {
        this->write_message_ = message;
        this->write_result_ = std::make_shared<std::promise<void>>();
        this->write_status_ = write_status_t::waiting_socket;
        this->change_mask(EPOLLOUT);
        return this->write_result_->get_future();
      }

      case write_status_t::continue_write: {
        this->write_message_ = message;
        this->write_result_ = std::make_shared<std::promise<void>>();
        this->write_data();
      }

      case write_status_t::eof:
        throw std::system_error(ECONNRESET, std::system_category());

      default:
        throw  std::runtime_error("Invalid operator");
      }
    }

    void read_data() {
      std::unique_lock<std::mutex> lock(this->read_mutex_);

      if (read_status_t::eof == this->read_status_) {
        auto result = std::move(this->read_result_);
        lock.unlock();

        if (result) {
          result->set_exception(std::make_exception_ptr(std::system_error(ECONNRESET, std::system_category())));
        }
        return;
      }

      if (read_status_t::waiting_socket != this->read_status_
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

      if (len <= 0) {
        int error = errno;
        if (EAGAIN == error) {
          this->read_status_ = read_status_t::waiting_socket;
          this->change_mask(EPOLLIN);
          return;
        }

        this->read_status_ = read_status_t::continue_read;
        auto result = std::move(this->read_result_);
        lock.unlock();

        result->set_exception(
          std::make_exception_ptr(std::system_error(error, std::system_category(), "recvfrom")));
      }
      else {
        this->sp_.get<logger>()->trace("UDP", this->sp_, "got %d bytes from %s", len,
          this->addr_.to_string().c_str());
        this->read_status_ = read_status_t::continue_read;
        auto result = std::move(this->read_result_);
        lock.unlock();

        result->set_value(_udp_datagram::create(this->addr_, this->read_buffer_, len));
      }
    }

    void write_data() {
      std::unique_lock<std::mutex> lock(this->write_mutex_);

      if (write_status_t::eof == this->write_status_) {
        auto result = std::move(this->write_result_);
        lock.unlock();

        if (result) {
          result->set_exception(
            std::make_exception_ptr(std::system_error(
              ECONNRESET, std::system_category())));
        }
        return;
      }

      if (write_status_t::waiting_socket != this->write_status_
        && write_status_t::continue_write != this->write_status_) {
        throw std::runtime_error("Invalid operation");
      }

      auto size = this->write_message_.data_size();
      int len = sendto(
        this->s_,
        this->write_message_.data(),
        size,
        0,
        this->write_message_.address(),
        this->write_message_.address().size());
      this->sp_.get<logger>()->trace("UDP", this->sp_, "send %d bytes to %s",
        size,
        this->write_message_.address().to_string().c_str());

      this->write_status_ = write_status_t::bof;
      if (len < 0) {
        int error = errno;
        if (EAGAIN == error) {
          this->write_status_ = write_status_t::waiting_socket;
          this->change_mask(EPOLLOUT);
          return;
        }

        auto result = std::move(this->write_result_);
        auto address = this->write_message_.address().to_string();
        lock.unlock();

        if (EMSGSIZE == error) {
          result->set_exception(std::make_exception_ptr(udp_datagram_size_exception()));
        }
        else {
          result->set_exception(std::make_exception_ptr(std::system_error(
            error,
            std::generic_category(),
            "Send to " + address)));
        }
      }
      else {
        if ((size_t)len != size) {
          throw std::runtime_error("Invalid send UDP");
        }

        this->sp_.get<logger>()->trace(
          "UDP",
          this->sp_,
          "Sent %d bytes to %s",
          len,
          this->write_message_.address().to_string().c_str());

        this->write_status_ = write_status_t::continue_write;
        auto result = std::move(this->write_result_);
        lock.unlock();

        result->set_value();
      }
    }

    void prepare_to_stop(const service_provider & sp)
    {
      std::unique_lock<std::mutex> lock1(this->write_mutex_);
      std::unique_lock<std::mutex> lock2(this->read_mutex_);

      if (this->read_result_) {
        this->read_result_->set_exception(
          std::make_exception_ptr(std::system_error(ECONNRESET, std::system_category())));
      }

      if (this->write_result_) {
        this->write_result_->set_exception(
          std::make_exception_ptr(std::system_error(ECONNRESET, std::system_category())));
      }

      this->read_status_ = read_status_t::eof;
      this->write_status_ = write_status_t::eof;
    }

    void stop() {
      base_class::stop();

      if (this->read_result_) {
        this->read_result_->set_exception(
          std::make_exception_ptr(vds_exceptions::shooting_down_exception()));
      }
      if (this->write_result_) {
        this->write_result_->set_exception(
          std::make_exception_ptr(vds_exceptions::shooting_down_exception()));
      }

      this->owner_.reset();
    }

  private:
    std::shared_ptr<_udp_socket> owner_;
    std::shared_ptr<std::promise<udp_datagram>> read_result_;
    std::shared_ptr<std::promise<void>> write_result_;

    network_address addr_;
    uint8_t read_buffer_[64 * 1024];

    udp_datagram write_message_;
  };

  std::shared_ptr<_udp_handler> handler_;
#endif//_WIN32

  class _udp_server
  {
  public:
    _udp_server(const network_address & address)
      : address_(address)
    {
    }

    std::tuple<std::shared_ptr<udp_datagram_reader>, std::shared_ptr<udp_datagram_writer>> & start(const service_provider & sp)
    {
      auto scope = sp.create_scope(("UDP server on " + this->address_.to_string()).c_str());
      imt_service::enable_async(scope);

      this->socket_ = udp_socket::create(scope, this->address_.family());

      if (0 > bind((*this->socket_)->handle(), this->address_, this->address_.size())) {
#ifdef _WIN32
        auto error = WSAGetLastError();
#else
        auto error = errno;
#endif
        throw std::system_error(error, std::system_category(), "bind socket");
      }

      return this->socket_->start(scope);
    }

    void prepare_to_stop(const service_provider & sp)
    {
      (*this->socket_)->prepare_to_stop(sp);
    }

    void stop(const service_provider & sp)
    {
      this->socket_->stop();
    }

    const std::shared_ptr<udp_socket> & socket() const { return this->socket_; }

    const network_address & address() const {
      return this->address_;
    }
  private:
    std::shared_ptr<udp_socket> socket_;
    network_address address_;
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

    std::tuple<std::shared_ptr<udp_datagram_reader>, std::shared_ptr<udp_datagram_writer>>
      start(const service_provider & sp, sa_family_t af)
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
      return this->socket_->start(sp);
    }


  private:
    std::shared_ptr<udp_socket> socket_;

  };
}

#endif//__VDS_NETWORK_UDP_SOCKET_P_H_
