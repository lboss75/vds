#ifndef __VDS_NETWORK_READ_SOCKET_TASK_H_
#define __VDS_NETWORK_READ_SOCKET_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "socket_task_p.h"
#include "network_service_p.h"

namespace vds {
  class inetwork_manager;
  
  class _read_socket_task : public _socket_task
  {
  public:
    constexpr static size_t BUFFER_SIZE = 1024;

    _read_socket_task(
      const std::function<void(const service_provider & sp, size_t readed)> & readed_method,
      const error_handler & error_method,
      SOCKET_HANDLE s)
    : sp_(service_provider::empty()),
      s_(s),
      readed_method_(readed_method), error_method_(error_method)
#ifdef _DEBUG
      , is_scheduled_(false)
#endif // _DEBUG
    {
    }

    ~_read_socket_task()
    {
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
          throw std::system_error(errorCode, std::system_category(), "WSARecv failed");
        }
      }
#else//!_WIN32
      this->buffer_ = buffer;
      this->buffer_size_ = buffer_size;
      if(nullptr == this->event_) {
        this->event_ = event_new(
          static_cast<_network_service *>(sp.get<inetwork_service>())->base_,
          this->s_,
          EV_READ,
          &_read_socket_task::callback,
          this);
      }
      // Schedule client event
      event_add(this->event_, NULL);
      static_cast<_network_service *>(sp.get<inetwork_service>())->start_libevent_dispatch(sp);
#endif//_WIN32
    }

  private:
    service_provider sp_;
    SOCKET_HANDLE s_;
    std::function<void(const service_provider & sp, size_t readed)> readed_method_;
    error_handler  error_method_;
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
#else//!_WIN32
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<_read_socket_task *>(arg);
      try {
        int len = read(fd, pthis->buffer_, pthis->buffer_size_);
        if (len < 0) {
          int error = errno;
          throw
            std::system_error(
              error,
            std::system_category());
        }
        
        imt_service::async(pthis->sp_, [pthis, len](){
          pthis->readed_method_(pthis->sp_, len);
        });
      }
      catch(...){
        pthis->error_method_(pthis->sp_, std::current_exception());
      }
    }
#endif//_WIN32
  };
}

#endif // __VDS_NETWORK_READ_SOCKET_TASK_H_
