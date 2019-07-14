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

    _udp_datagram(const _udp_datagram & other) = default;
    _udp_datagram(_udp_datagram && other) = default;

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
    _udp_socket(
        const service_provider * sp,
        SOCKET_HANDLE s,
        sa_family_t family)
      : sp_(sp),
        s_(s),
        family_(family)
#ifndef _WIN32
        , event_masks_(0)
#endif//_WIN32
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

    sa_family_t family() const {
      return this->family_;
    }

#ifndef _WIN32
    std::tuple<
        std::shared_ptr<vds::udp_datagram_reader>,
            std::shared_ptr<vds::udp_datagram_writer>>
    start(
        const service_provider * sp,
        const std::shared_ptr<socket_base> & owner);

        expected<void> process(uint32_t events);

    expected<void>  change_mask(
        const std::shared_ptr<socket_base> & owner,
        uint32_t set_events,
        uint32_t clear_events = 0)
    {
      std::unique_lock<std::mutex> lock(this->event_masks_mutex_);
      auto last_mask = this->event_masks_;
      this->event_masks_ |= set_events;
      this->event_masks_ &= ~clear_events;

      if(last_mask == this->event_masks_){
          return expected<void>();
      }

      if(0 != last_mask && 0 != this->event_masks_){
        CHECK_EXPECTED((*this->sp_->get<network_service>())->set_events(this->s_, this->event_masks_));
      }
      else if (0 == this->event_masks_){
          CHECK_EXPECTED((*this->sp_->get<network_service>())->remove_association(this->s_));
      }
      else {
          CHECK_EXPECTED((*this->sp_->get<network_service>())->associate(this->s_, owner, this->event_masks_));
      }
      return expected<void>();
    }

#endif//_WIN32

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

  private:
    const service_provider * sp_;
    SOCKET_HANDLE s_;
    sa_family_t family_;

#ifndef _WIN32
    std::mutex event_masks_mutex_;
    uint32_t event_masks_;

    std::weak_ptr<class _udp_receive> read_task_;
    std::weak_ptr<class _udp_send> write_task_;

#endif
  };

#ifdef _WIN32
  class _udp_receive : public _socket_task, public udp_datagram_reader
  {
  public:
    _udp_receive(
        const service_provider * sp,
        const std::shared_ptr<udp_socket> & s)
      : sp_(sp), s_(s)
    {
    }

    vds::async_task<vds::expected<udp_datagram>> read_async()
    {
      vds_assert(!this->result_);

      memset(&this->overlapped_, 0, sizeof(this->overlapped_));
      this->wsa_buf_.len = sizeof(this->buffer_);
      this->wsa_buf_.buf = (CHAR *)this->buffer_;
      this->addr_.clear();

      auto r = std::make_shared<vds::async_result<vds::expected<udp_datagram>>>();
      this->result_ = r;

      this->sp_->get<logger>()->trace("UDP", "WSARecvFrom %d", (*this->s_)->handle());

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
          r->set_value(
            make_unexpected<std::system_error>(
                errorCode,
                std::system_category(),
                "WSARecvFrom failed"));
        }
        else {
          this->sp_->get<logger>()->trace("UDP", "Read scheduled");
        }
      }
      else {
        auto errorCode = WSAGetLastError();
        this->sp_->get<logger>()->trace("UDP", "Direct readed %d, code %d", numberOfBytesRecvd, errorCode);
        //this_->process(numberOfBytesRecvd);
      }

      return r->get_future();
    }

    void prepare_to_stop()
    {
    }

  private:
    const service_provider * sp_;
    std::shared_ptr<udp_socket> s_;
    std::shared_ptr<vds::async_result<vds::expected<udp_datagram>>> result_;

    network_address addr_;
    uint8_t buffer_[64 * 1024];

    void process(DWORD dwBytesTransfered) override
    {
      this->sp_->get<logger>()->trace("UDP", "Got %d bytes UDP package from %s", dwBytesTransfered, this->addr_.to_string().c_str());

      vds_assert(this->result_);
      auto pthis = this->shared_from_this();
      auto r = std::move(this->result_);
      r->set_value(_udp_datagram::create(this->addr_, this->buffer_, (size_t)dwBytesTransfered));
    }

    void error(DWORD error_code) override
    {
      this->sp_->get<logger>()->trace("UDP", "Error %d at get recive UDP package", error_code);

      auto r = std::move(this->result_);
      r->set_value(make_unexpected<std::system_error>(error_code, std::system_category(), "WSARecvFrom failed"));
    }
  };

  class _udp_send : public _socket_task, public udp_datagram_writer
  {
  public:
    _udp_send(
        const service_provider * sp,
        const std::shared_ptr<udp_socket> & s)
      : sp_(sp), s_(s) {
    }

    vds::async_task<vds::expected<void>> write_async(const udp_datagram & data)
    {
      vds_assert(!this->result_);
      memset(&this->overlapped_, 0, sizeof(this->overlapped_));
      this->buffer_ = data;
      this->wsa_buf_.len = this->buffer_.data_size();
      this->wsa_buf_.buf = (CHAR *)this->buffer_.data();
      this->sp_->get<logger>()->trace(
        "UDP",
        "write_async %s %d bytes",
        this->buffer_->address().to_string().c_str(),
        this->buffer_.data_size());

      auto r = std::make_shared<vds::async_result<vds::expected<void>>>();
      this->result_ = r;


      this->sp_->get<logger>()->trace(
        "UDP",
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
          this->sp_->get<logger>()->trace(
            "UDP",
            "Error %d at schedule sending UDP to %s",
            errorCode,
            this->buffer_->address().to_string().c_str());

          r->set_value(make_unexpected<std::system_error>(errorCode, std::system_category(), "WSASend failed"));
          this->result_.reset();
        }
      }

      return r->get_future();
    }

    void prepare_to_stop()
    {
    }

  private:
    const service_provider * sp_;
    std::shared_ptr<udp_socket> s_;
    std::shared_ptr<vds::async_result<vds::expected<void>>> result_;

    socklen_t addr_len_;
    udp_datagram buffer_;

    void process(DWORD dwBytesTransfered) override
    {
      this->sp_->get<logger>()->trace(
        "UDP",
        "Sent %d bytes UDP package to %s",
        dwBytesTransfered,
        this->buffer_->address().to_string().c_str());

      vds_assert(this->result_);
      auto result = std::move(this->result_);

      if (this->wsa_buf_.len != (size_t)dwBytesTransfered) {
        result->set_value(make_unexpected<std::runtime_error>("Invalid sent UDP data"));
      }
      else {
        result->set_value(expected<void>());
      }
    }

    void error(DWORD error_code) override
    {
      this->sp_->get<logger>()->trace(
        "UDP",
        "Error %d at sending UDP to %s",
        error_code,
        this->buffer_->address().to_string().c_str());

      vds_assert(this->result_);
      auto result = std::move(this->result_);

      result->set_value(make_unexpected<std::system_error>(error_code, std::system_category(), "WSASendTo failed"));
    }
  };

#else
  class _udp_receive : public udp_datagram_reader
  {
  public:
    _udp_receive(
        const service_provider * sp,
        const std::shared_ptr<socket_base> & owner)
      : sp_(sp),
        owner_(owner)
    {
    }

    ~_udp_receive()
    {
    }

    vds::async_task<vds::expected<udp_datagram>> read_async() {
      auto r = std::make_shared<vds::async_result<vds::expected<udp_datagram>>>();

      this->addr_.reset();
      int len = recvfrom((*this->owner())->handle(),
                         this->read_buffer_,
                         sizeof(this->read_buffer_),
                         0,
                         this->addr_,
                         this->addr_.size_ptr());

      if (len <= 0) {
        int error = errno;
        if (EAGAIN == error) {
          this->read_result_ = r;
          CHECK_EXPECTED((*this->owner())->change_mask(this->owner_, EPOLLIN));
        }
        else {
          this->sp_->get<logger>()->trace("UDP", "Error %d at get recive UDP package", error);
          r->set_value(make_unexpected<std::system_error>(error, std::system_category(), "recvfrom"));
        }
      }
      else {
        this->sp_->get<logger>()->trace("UDP", "Got %d bytes UDP package from %s", len, this->addr_.to_string().c_str());
        r->set_value(_udp_datagram::create(this->addr_, this->read_buffer_, len));
      }

      return r->get_future();
    }


    expected<void> process() {
      this->addr_.reset();
      int len = recvfrom((*this->owner())->handle(),
                         this->read_buffer_,
                         sizeof(this->read_buffer_),
                         0,
                         this->addr_,
                         this->addr_.size_ptr());

      auto r = std::move(this->read_result_);
      if (len <= 0) {
        int error = errno;
        if (EAGAIN == error) {
          return expected<void>();
        }

        CHECK_EXPECTED((*this->owner())->change_mask(this->owner_, 0, EPOLLIN));
        this->sp_->get<logger>()->trace("UDP", "Error %d at get recive UDP package", error);
        r->set_value(
          make_unexpected<std::system_error>(error, std::system_category(), "recvfrom"));
      }
      else {
        this->sp_->get<logger>()->trace(
            "UDP",
            "Got %d bytes UDP package from %s",
            len,
            this->addr_.to_string().c_str());

          CHECK_EXPECTED((*this->owner())->change_mask(this->owner_, 0, EPOLLIN));
        r->set_value(_udp_datagram::create(this->addr_, this->read_buffer_, len));
      }
      return expected<void>();
    }


  private:
    const service_provider * sp_;
    std::shared_ptr<socket_base> owner_;
    std::shared_ptr<vds::async_result<vds::expected<udp_datagram>>> read_result_;

    network_address addr_;
    uint8_t read_buffer_[64 * 1024];

    udp_socket * owner() const {
      return static_cast<udp_socket *>(this->owner_.get());
    }
  };

  class _udp_send : public udp_datagram_writer {
  public:
    _udp_send(
        const service_provider * sp,
        const std::shared_ptr<socket_base> & owner)
        : sp_(sp),
          owner_(owner) {

    }

    vds::async_task<vds::expected<void>> write_async( const udp_datagram & message) {
      auto r = std::make_shared<vds::async_result<vds::expected<void>>>();
      int len = sendto(
          (*this->owner())->handle(),
          message.data(),
          message.data_size(),
          0,
          message.address(),
          message.address().size());

      if (len < 0) {
        int error = errno;
        if (EAGAIN == error) {
          this->write_message_ = message;
          this->write_result_ = r;
          CHECK_EXPECTED((*this->owner())->change_mask(
              this->owner_, EPOLLOUT));
        }
        else {
          auto address = message.address().to_string();

          this->sp_->get<logger>()->trace(
            "UDP",
            "Error %d at sending UDP to %s",
            error,
            address .c_str());

          r->set_value(make_unexpected<std::system_error>(
            error,
            std::generic_category(),
            "Send to " + address));

        }
      }
      else {
        if ((size_t)len != message.data_size()) {
          r->set_value(make_unexpected<std::runtime_error>("Invalid send UDP"));
        }
        else {
          this->sp_->get<logger>()->trace(
            "UDP",
            "Sent %d bytes UDP package to %s",
            message.data_size(),
            message.address().to_string().c_str());
          r->set_value(expected<void>());
        }
      }

      return r->get_future();
    }

    expected<void> process(){

      auto size = this->write_message_.data_size();
      int len = sendto(
          (*this->owner())->handle(),
          this->write_message_.data(),
          size,
          0,
          this->write_message_.address(),
          this->write_message_.address().size());

      auto result = std::move(this->write_result_);
      if (len < 0) {
        int error = errno;
        if (EAGAIN == error) {
          return expected<void>();
        }

        CHECK_EXPECTED((*this->owner())->change_mask(this->owner_, 0, EPOLLOUT));
        this->sp_->get<logger>()->trace(
          "UDP",
          "Error %d at sending UDP to %s",
          error,
          this->write_message_.address().to_string().c_str());


        auto address = this->write_message_.address().to_string();

        result->set_value(make_unexpected<std::system_error>(
              error,
              std::generic_category(),
              "Send to " + address));
      }
      else {
          CHECK_EXPECTED((*this->owner())->change_mask(this->owner_, 0, EPOLLOUT));

        this->sp_->get<logger>()->trace(
          "UDP",
          "Sent %d bytes UDP package to %s",
          this->write_message_.data_size(),
          this->write_message_.address().to_string().c_str());
        if ((size_t)len != size) {
          result->set_value(make_unexpected<std::runtime_error>("Invalid send UDP"));
        }
        else {
          result->set_value(expected<void>());
        }
      }
      return expected<void>();
    }

  private:
    const service_provider * sp_;
    std::shared_ptr<socket_base> owner_;
    std::shared_ptr<vds::async_result<vds::expected<void>>> write_result_;
    udp_datagram write_message_;

    udp_socket * owner() const {
      return static_cast<udp_socket *>(this->owner_.get());
    }
  };
#endif//_WIN32

  class _udp_server
  {
  public:
    _udp_server(const network_address & address)
      : address_(address)
    {
    }

    expected<std::tuple<std::shared_ptr<udp_datagram_reader>, std::shared_ptr<udp_datagram_writer>>> start(const service_provider * sp)
    {
      GET_EXPECTED_VALUE(this->socket_, udp_socket::create(sp, this->address_.family()));

      if (0 > bind((*this->socket_)->handle(), this->address_, this->address_.size())) {
#ifdef _WIN32
        auto error = WSAGetLastError();
#else
        auto error = errno;
#endif
        return vds::make_unexpected<std::system_error>(error, std::system_category(), "bind socket");
      }

      return this->socket_->start(sp);
    }

    void prepare_to_stop()
    {
    }

    void stop()
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

    expected<std::tuple<std::shared_ptr<udp_datagram_reader>, std::shared_ptr<udp_datagram_writer>>>
      start(const service_provider * sp, sa_family_t af)
    {
      GET_EXPECTED_VALUE(this->socket_, udp_socket::create(sp, af));
      return this->socket_->start(sp);
    }


  private:
    std::shared_ptr<udp_socket> socket_;

  };
}

#endif//__VDS_NETWORK_UDP_SOCKET_P_H_
