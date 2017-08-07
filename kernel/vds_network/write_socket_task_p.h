#ifndef __VDS_NETWORK_WRITE_SOCKET_TASK_H_
#define __VDS_NETWORK_WRITE_SOCKET_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "socket_task_p.h"
#include "network_service_p.h"

namespace vds {
  class network_service;
  
  class _write_socket_task : public _socket_task
  {
  public:
    _write_socket_task(
      const std::function<void(const service_provider & sp, size_t written)> & written_method,
      const error_handler & error_method,
      SOCKET_HANDLE s,
      const cancellation_token & cancel_token)
    : sp_(service_provider::empty()),
      written_method_(written_method),
      error_method_(error_method),
      s_(s),
      cancel_token_(cancel_token)
#ifdef _DEBUG
      , is_scheduled_(false)
#endif
    {
      this->cancel_subscriber_ = this->cancel_token_.then_cancellation_requested([this]() {
        shutdown(this->s_, SD_BOTH);
      });
    }

    ~_write_socket_task()
    {
      this->cancel_subscriber_.destroy();

#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw std::exception();
      }
#endif

    }
    
    void write_async(const service_provider & sp, const void * buffer, size_t buffer_size)
    {
      if (0 == buffer_size) {
        shutdown(this->s_, SD_SEND);
        mt_service::async(sp, [this, sp]() {
          this->written_method_(sp, 0);
        });
        return;
      }

      this->sp_ = sp;

#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw std::exception();
      }
      this->is_scheduled_ = true;
#endif
      
#ifdef _WIN32
      this->wsa_buf_.len = (ULONG)buffer_size;
      this->wsa_buf_.buf = (CHAR *)buffer;

      if (NOERROR != WSASend(this->s_, &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
          this->is_scheduled_ = false;
          throw std::system_error(errorCode, std::system_category(), "WSASend failed");
        }
      }
#else
      this->data_ = buffer;
      this->data_size_ = buffer_size;
      
      auto service = static_cast<_network_service *>(sp.get<inetwork_service>());
      service->start_write(
        this->s_,
        this                                                                     
      );
      service->start_dispatch(sp);
#endif
    }
  
  private:
    service_provider sp_;
    std::function<void(const service_provider & sp, size_t written)> written_method_;
    error_handler error_method_;
    SOCKET_HANDLE s_;
    cancellation_token cancel_token_;
    cancellation_subscriber cancel_subscriber_;

#ifndef _WIN32
    const void * data_;
    size_t data_size_;
#endif
    
#ifdef _DEBUG
    bool is_scheduled_;
#endif


#ifdef _WIN32
    void process(DWORD dwBytesTransfered) override
    {
      try {
#ifdef _DEBUG
        if (!this->is_scheduled_) {
          throw std::exception();
        }
        this->is_scheduled_ = false;
#endif
        this->written_method_(this->sp_, dwBytesTransfered);
      }
      catch (const std::exception & ex) {
        this->error_method_(this->sp_, std::make_shared<std::exception>(ex));
      }
      catch (...) {
        this->error_method_(this->sp_, std::make_shared<std::runtime_error>("Unexpected error"));
      }
    }
    void error(DWORD error_code) override
    {
      this->error_method_(this->sp_, std::make_unique<std::system_error>(error_code, std::system_category(), "write to tcp socket"));
    }

#else//!_WIN32
    void process() override
    {
      logger::get(this->sp_)->trace(this->sp_, "write %d bytes", this->data_size_);
      try {
        int len = write(this->s_, this->data_, this->data_size_);
        if (len < 0) {
          int error = errno;
          throw std::system_error(
              error,
              std::system_category());
        }
        
        imt_service::async(this->sp_,
          [this, len](){
            this->written_method_(this->sp_, len);
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

#endif // __VDS_NETWORK_WRITE_SOCKET_TASK_H_
