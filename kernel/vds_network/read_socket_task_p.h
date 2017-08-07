#ifndef __VDS_NETWORK_READ_SOCKET_TASK_H_
#define __VDS_NETWORK_READ_SOCKET_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "socket_task_p.h"
#include "network_service_p.h"
#include "tcp_network_socket_p.h"

namespace vds {
  class inetwork_service;
  
  class _read_socket_task : public _socket_task
  {
  public:
    constexpr static size_t BUFFER_SIZE = 1024;

    _read_socket_task(
      const std::function<void(const service_provider & sp, size_t readed)> & readed_method,
      const error_handler & error_method,
      SOCKET_HANDLE s,
      const cancellation_token & cancel_token)
    : sp_(service_provider::empty()),
      s_(s),
      readed_method_(readed_method),
      error_method_(error_method),
      cancel_token_(cancel_token)
#ifdef _DEBUG
      , is_scheduled_(false)
#endif // _DEBUG
    {
      this->cancel_subscriber_ = this->cancel_token_.then_cancellation_requested([this]() {
        shutdown(this->s_, SD_BOTH);
        _tcp_network_socket(this->s_).close();
      });
    }

    ~_read_socket_task()
    {
      this->cancel_subscriber_.destroy();
#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw std::runtime_error("");
      }
#endif // _DEBUG
    }
    
    void read_async(const service_provider & sp, void * buffer, size_t buffer_size)
    {
      this->sp_ = sp;

#ifdef _WIN32
      this->wsa_buf_.len = buffer_size;
      this->wsa_buf_.buf = (CHAR *)buffer;

#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw std::exception();
      }
      this->is_scheduled_ = true;
#endif
      DWORD flags = 0;
      DWORD numberOfBytesRecvd;
      if (NOERROR != WSARecv(this->s_, &this->wsa_buf_, 1, &numberOfBytesRecvd, &flags, &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
          this->is_scheduled_ = false;

          if (this->cancel_token_.is_cancellation_requested() && (WSAESHUTDOWN == errorCode)) {
            this->readed_method_(this->sp_, 0);
          }
          else {
            this->error_method_(this->sp_, std::make_unique<std::system_error>(errorCode, std::system_category(), "read from tcp socket"));
          }
          return;
        }
      }
#else//!_WIN32
      this->buffer_ = buffer;
      this->buffer_size_ = buffer_size;
      
      auto service = static_cast<_network_service *>(sp.get<inetwork_service>());
      service->start_read(
        this->s_,
        this                                                                     
      );
      service->start_dispatch(sp);
#endif//_WIN32
    }

  private:
    service_provider sp_;
    SOCKET_HANDLE s_;
    std::function<void(const service_provider & sp, size_t readed)> readed_method_;
    error_handler  error_method_;
    cancellation_token cancel_token_;
    cancellation_subscriber cancel_subscriber_;
#ifndef _WIN32
    void * buffer_;
    size_t buffer_size_;
#endif

#ifdef _DEBUG
    bool is_scheduled_;
#endif
    
#ifdef _WIN32
    void process(DWORD dwBytesTransfered) override
    {
#ifdef _DEBUG
      if (!this->is_scheduled_) {
        throw std::exception();
      }
      this->is_scheduled_ = false;
#endif

      this->readed_method_(
        this->sp_,
        (size_t)dwBytesTransfered
      );
    }

    void error(DWORD error_code) override
    {
      if (this->cancel_token_.is_cancellation_requested() && (WSAESHUTDOWN == error_code || ERROR_OPERATION_ABORTED == error_code)) {
        this->process(0);
      }
      else {
        this->error_method_(this->sp_, std::make_unique<std::system_error>(error_code, std::system_category(), "read from tcp socket"));
      }
    }

#else//!_WIN32
    void process() override
    {
      try {
        int len = read(this->s_, this->buffer_, this->buffer_size_);
        if (len < 0) {
          int error = errno;
          throw
            std::system_error(
              error,
            std::system_category());
        }
        
        imt_service::async(this->sp_, [this, len](){
          this->readed_method_(this->sp_, len);
        });
      }
      catch (const std::exception & ex) {
        this->error_method_(this->sp_, std::make_shared<std::exception>(ex));
      }
      catch (...) {
        this->error_method_(this->sp_, std::make_shared<std::runtime_error>("Unexpected error"));
      }
    }
#endif//_WIN32
  };
}

#endif // __VDS_NETWORK_READ_SOCKET_TASK_H_
