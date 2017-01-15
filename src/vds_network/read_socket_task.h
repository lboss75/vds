#ifndef __VDS_NETWORK_READ_SOCKET_TASK_H_
#define __VDS_NETWORK_READ_SOCKET_TASK_H_

#include "socket_task.h"

namespace vds {
  class inetwork_manager;
  
  template<
    typename done_method_type,
    typename error_method_type
  >
  class read_socket_task : public socket_task
  {
  public:
    constexpr static size_t BUFFER_SIZE = 1024;
    
#ifdef _WIN32
    void process(DWORD dwBytesTransfered) override
    {
      this->done_method_(
        this->buffer_,
        (size_t)dwBytesTransfered
      );
    }
#else//!_WIN32
    void schedule()
    {
      event_set(
        &this->event_,
        this->s_,
        EV_READ,
        &read_socket_task::callback,
        this);
      // Schedule client event
      event_add(&this->event_, NULL);
    }
#endif//_WIN32

    
  private:
    done_method_type done_method_;
    error_method_type error_method_;
    u_int8_t buffer_[BUFFER_SIZE];
    
#ifdef _WIN32
#else//!_WIN32
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<read_socket_task *>(arg);
      int len = read(fd, pthis->buffer_, BUFFER_SIZE);
      if (len < 0) {
        int error = errno;
        pthis->error_method_(
          new std::system_error(
            error,
            std::system_category()));
        return;
      }
      
      pthis->done_method_(pthis->buffer_, len);
    }
#endif//_WIN32

  };

}

#endif // __VDS_NETWORK_READ_SOCKET_TASK_H_
