#ifndef __VDS_NETWORK_WRITE_SOCKET_TASK_H_
#define __VDS_NETWORK_WRITE_SOCKET_TASK_H_

#include "socket_task.h"

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
      const done_method_type & done_method,
      const error_method_type & error_method)
    : done_method_(done_method), error_method_(error_method),
      data_(nullptr), data_size_(0)      
    {
    }
   
#ifdef _WIN32
    void process(DWORD dwBytesTransfered) override
    {
      this->data_ += dwBytesTransfered;
      this->data_size_ -= dwBytesTransfered;

      if (this->data_size_ > 0) {
        this->schedule();
      }
      else {
        this->done_method_();
      }
    }
    
    void schedule()
    {
      this->wsa_buf_.len = this->data_size_;
      this->wsa_buf_.buf = (CHAR *)this->data_;

      if (NOERROR != WSASend(this->s_, &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
          throw new windows_exception("WSASend failed", errorCode);
        }
      }
    }
#else//!_WIN32
    void schedule()
    {
      event_set(
        &this->event_,
        this->s_,
        EV_WRITE,
        &write_socket_task::callback,
        this);
      // Schedule client event
      event_add(&this->event_, NULL);
    }
#endif//_WIN32

    
  private:
    done_method_type done_method_;
    error_method_type error_method_;
    network_socket::SOCKET_HANDLE s_;
    const u_int8_t * data_;
    size_t data_size_;
    
#ifdef _WIN32
#else//!_WIN32
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<write_socket_task *>(arg);
      int len = write(fd, pthis->data_, pthis->data_size_);
      if (len < 0) {
        int error = errno;
        pthis->error_method_(
          new std::system_error(
            error,
            std::system_category()));
        return;
      }
      
      pthis->data_ += len;
      pthis->data_size_ -= len;
      if (0 < pthis->data_size_) {
        event_set(&pthis->event_, pthis->s_, EV_WRITE, &write_socket_task::callback, pthis);
        event_add(&pthis->event_, NULL);
      }
      else {
        pthis->done_method_();
      }
    }
#endif//_WIN32

  };

}

#endif // __VDS_NETWORK_WRITE_SOCKET_TASK_H_
