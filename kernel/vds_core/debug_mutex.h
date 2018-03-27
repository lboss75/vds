#ifndef __VDS_CORE_DEBUG_MUTEX_H_
#define __VDS_CORE_DEBUG_MUTEX_H_

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
#else
#include <minwindef.h>
#include <processthreadsapi.h>
#endif//_WIN32

namespace std {
  class debug_mutex : public mutex
  {
  public:
    debug_mutex()
      : owner_id_(0)
    {
    }

    ~debug_mutex()
    {
#ifdef _DEBUG
#pragma warning(disable: 4297)

      if (0 != this->owner_id_) {
        throw std::runtime_error("Multithreading error");
      }
#pragma warning(default: 4297)
#endif//_DEBUG
    }

    void lock()
    {
      mutex::lock();
#ifndef _WIN32
      this->owner_id_ = syscall(SYS_gettid);
#else
      this->owner_id_ = GetCurrentThreadId();
#endif
    }

    void unlock()
    {
      this->owner_id_ = 0;
      mutex::unlock();
    }

  private:
#ifndef _WIN32
    pid_t owner_id_;
#else
    DWORD owner_id_;
#endif//_WIN32
  };
}

#if !defined(_DEBUG) || !defined(DEBUG)
#define debug_mutex mutex
#endif


#endif//__VDS_CORE_DEBUG_MUTEX_H_
