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
      SOCKET_HANDLE s)
    : sp_(service_provider::empty()),
      written_method_(written_method),
      error_method_(error_method)
#ifdef _DEBUG
      , is_scheduled_(false)
#endif
    {
    }

    ~_write_socket_task()
    {
#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw std::exception();
      }
#endif

    }
    
    void write_async(const service_provider & sp, const void * buffer, size_t buffer_size)
    {
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
      if(nullptr == this->event_) {
        this->event_ = event_new(
          static_cast<_network_service *>(sp.get<inetwork_service>())->base_,
          this->s_,
          EV_WRITE,
          &_write_socket_task::callback,
          this);
      }
      // Schedule client event
      event_add(this->event_, NULL);
      
      static_cast<_network_service *>(sp.get<inetwork_service>())->start_libevent_dispatch(sp);
#endif
    }
  
  private:
    service_provider sp_;
    std::function<void(const service_provider & sp, size_t written)> written_method_;
    error_handler error_method_;
    SOCKET_HANDLE s_;
    
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
        this->data_ += dwBytesTransfered;
        this->data_size_ -= dwBytesTransfered;

        if (this->data_size_ > 0) {
          this->schedule(this->sp_);
        }
        else {
          this->done_method_(this->sp_);
        }
      }
      catch (...) {
        this->error_method_(this->sp_, std::current_exception());
      }
    }
#else//!_WIN32
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<_write_socket_task *>(arg);
      logger::get(pthis->sp_)->trace(pthis->sp_, "write %d bytes", pthis->data_size_);
      try {
        int len = write(fd, pthis->data_, pthis->data_size_);
        if (len < 0) {
          int error = errno;
          throw std::system_error(
              error,
              std::system_category());
        }
        
        imt_service::async(pthis->sp_,
          [pthis, len](){
            pthis->written_method_(pthis->sp_, len);
          });
      }
      catch(...){
        pthis->error_method_(pthis->sp_, std::current_exception());
      }
    }
#endif//_WIN32
  };

}

#endif // __VDS_NETWORK_WRITE_SOCKET_TASK_H_
