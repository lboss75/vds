#ifndef __VDS_CORE_NOT_MUTEX_H_
#define __VDS_CORE_NOT_MUTEX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#endif//_WIN32

namespace vds {
  class not_mutex
  {
  public:
    not_mutex()
      : is_locked_(false)
    {
    }

    void lock()
    {
      std::unique_lock<std::mutex> lock(this->m_);

      if(this->is_locked_) {
        std::cout << "This object owned by thread " << this->owner_id_ << "\n";
        throw std::runtime_error("Multithreading error");
      }

      this->is_locked_ = true;
#ifndef _WIN32
      this->owner_id_ = syscall(SYS_gettid);
#else
      this->owner_id_ = GetCurrentThreadId();
#endif
    }

    void unlock()
    {
      std::unique_lock<std::mutex> lock(this->m_);

      if (!this->is_locked_) {
        throw std::runtime_error("Multithreading error");
      }

      this->is_locked_ = false;
    }

  private:
    std::mutex m_;
    bool is_locked_;
#ifndef _WIN32
    pid_t owner_id_;
#else
    DWORD owner_id_;
#endif//_WIN32
  };
}



#endif//__VDS_CORE_NOT_MUTEX_H_
