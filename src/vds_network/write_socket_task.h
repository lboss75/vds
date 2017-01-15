#ifndef __VDS_NETWORK_WRITE_SOCKET_TASK_H_
#define __VDS_NETWORK_WRITE_SOCKET_TASK_H_

#include "socket_task.h"

namespace vds {
  
  template<
    typename done_method_type,
    typename error_method_type
  >
  class write_socket_task : public socket_task
  {
  public:
   
#ifdef _WIN32
    void process(DWORD dwBytesTransfered) override
    {
      this->done_method_(
        this->buffer_,
        (size_t)dwBytesTransfered
      );
    }
#else//!_WIN32
    void schedule(network_manager *)
    {
      event_set(
        &this->event_,
        this->s_,
        EV_READ,
        &write_socket_task::callback,
        this);
      // Schedule client event
      event_add(&this->event_, NULL);
    }
#endif//_WIN32

    
  private:
    done_method_type done_method_;
    error_method_type error_method_;
    uint_8 buffer_[BUFFER_SIZE];
    
#ifdef _WIN32
#else//!_WIN32
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<write_socket_task *>(arg);
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

#endif // __VDS_NETWORK_WRITE_SOCKET_TASK_H_
