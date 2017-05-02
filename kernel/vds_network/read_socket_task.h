#ifndef __VDS_NETWORK_READ_SOCKET_TASK_H_
#define __VDS_NETWORK_READ_SOCKET_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "socket_task.h"
#include "network_socket.h"

namespace vds {
  class inetwork_manager;
  
  template<
    typename next_method_type,
    typename error_method_type
  >
  class read_socket_task : public socket_task
  {
  public:
    constexpr static size_t BUFFER_SIZE = 1024;

    read_socket_task(
      const service_provider & sp,
      next_method_type & next_method,
      error_method_type & error_method,
      const network_socket & s
    ) : sp_(sp),
    network_service_((network_service *)&sp.get<inetwork_manager>()),
    s_(s.handle()),
    next_method_(next_method), error_method_(error_method)
#ifdef _DEBUG
      , is_scheduled_(false)
#endif // _DEBUG
    {
    }

    ~read_socket_task()
    {
#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw new std::runtime_error("");
      }
#endif // _DEBUG
    }
    
    void operator()(const service_provider & sp)
    {
#ifdef _WIN32
      this->wsa_buf_.len = BUFFER_SIZE;
      this->wsa_buf_.buf = (CHAR *)this->buffer_;

#ifdef _DEBUG
      if (this->is_scheduled_) {
        throw new std::exception();
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
      if(nullptr == this->event_) {
        this->event_ = event_new(
          this->network_service_->base_,
          this->s_,
          EV_READ,
          &read_socket_task::callback,
          this);
      }
      // Schedule client event
      event_add(this->event_, NULL);
      this->network_service_->start_libevent_dispatch(this->sp_);
#endif//_WIN32
    }

  private:
    service_provider sp_;
    network_service * network_service_;
    network_socket::SOCKET_HANDLE s_;
    next_method_type & next_method_;
    error_method_type & error_method_;
    uint8_t buffer_[BUFFER_SIZE];
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

      this->next_method_(
        this->sp_,
        this->buffer_,
        (size_t)dwBytesTransfered
      );
    }
#else//!_WIN32
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<read_socket_task *>(arg);
      try {
        int len = read(fd, pthis->buffer_, BUFFER_SIZE);
        if (len < 0) {
          int error = errno;
          throw
            std::system_error(
              error,
            std::system_category());
        }
        
        imt_service::async(pthis->sp_, [pthis, len](){
            pthis->next_method_(pthis->sp_, pthis->buffer_, len);
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
