#ifndef __VDS_NETWORK_WRITE_SOCKET_TASK_H_
#define __VDS_NETWORK_WRITE_SOCKET_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "socket_task.h"
#include "network_socket.h"

namespace vds {
  class network_service;
  
  template<
    typename done_method_type,
    typename error_method_type
  >
  class write_socket_task : public socket_task
  {
  public:
    write_socket_task(
      done_method_type & done_method,
      error_method_type & error_method
      )
    : sp_(service_provider::empty()),
      done_method_(done_method), error_method_(error_method),
      data_(nullptr), data_size_(0)
#ifdef _DEBUG
      , is_scheduled_(false)
#endif
    {
    }

    ~write_socket_task()
    {
#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw new std::exception();
      }
#endif

    }

    void set_data(
      const void * data,
      size_t size
    )
    {
      this->data_ = static_cast<const uint8_t *>(data);
      this->data_size_ = size;
    }

    void schedule(const service_provider & sp, network_socket::SOCKET_HANDLE s)
    {
      this->sp_ = sp;
      this->s_ = s;
      this->schedule(sp);
    }
  
  private:
    service_provider sp_;
    done_method_type & done_method_;
    error_method_type & error_method_;
    network_socket::SOCKET_HANDLE s_;
    const uint8_t * data_;
    size_t data_size_;
#ifdef _DEBUG
    bool is_scheduled_;
#endif

#ifdef _WIN32
    void schedule(const service_provider & sp)
    {
#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw new std::exception();
      }
      this->is_scheduled_ = true;
#endif
      this->wsa_buf_.len = (ULONG)this->data_size_;
      this->wsa_buf_.buf = (CHAR *)this->data_;

      if (NOERROR != WSASend(this->s_, &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
          this->is_scheduled_ = false;
          throw new std::system_error(errorCode, std::system_category(), "WSASend failed");
        }
      }
    }

    void process(DWORD dwBytesTransfered) override
    {
      try {
#ifdef _DEBUG
        if (!this->is_scheduled_) {
          throw new std::exception();
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
    void schedule(const service_provider & sp)
    {
      if(nullptr == this->event_) {
        this->event_ = event_new(
          this->network_service_->base_,
          this->s_,
          EV_WRITE,
          &write_socket_task::callback,
          this);
      }
      // Schedule client event
      event_add(this->event_, NULL);
      
      this->network_service_->start_libevent_dispatch(sp);
    }
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<write_socket_task *>(arg);
      try {
        int len = write(fd, pthis->data_, pthis->data_size_);
        if (len < 0) {
          int error = errno;
          throw std::system_error(
              error,
              std::system_category());
        }
        
        pthis->data_ += len;
        pthis->data_size_ -= len;
        if (0 < pthis->data_size_) {
          //event_set(&pthis->event_, pthis->s_, EV_WRITE, &write_socket_task::callback, pthis);
          event_add(pthis->event_, NULL);
        }
        else {
          imt_service::async(pthis->sp_,
            [pthis](){
              pthis->done_method_(pthis->sp_);
            });
        }
      }
      catch(...){
        pthis->error_method_(pthis->sp_, std::current_exception());
      }
    }
#endif//_WIN32

  };

}

#endif // __VDS_NETWORK_WRITE_SOCKET_TASK_H_
