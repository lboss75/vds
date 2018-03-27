#ifndef __VDS_CORE_LEGACY_H_
#define __VDS_CORE_LEGACY_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#if defined(_MSC_VER) && (_MSC_VER < 1800 || defined(__cplusplus))
#define __cplusplus_version 201402L
#else
#define __cplusplus_version __cplusplus
#endif

//C++11
#if __cplusplus_version < 201103L
  #error "C++11 is required"
#endif

//C++14
#if __cplusplus_version <=  201402L

//#pragma message ("Using legacy code")
#include <mutex>
#include <condition_variable>

#ifndef _WIN32
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#else
#include <minwindef.h>
#include <processthreadsapi.h>
#endif//_WIN32

namespace std {
  class shared_mutex
  {
  public:

    shared_mutex()
    : state_(0), owner_id_(0)
    {}

    // Exclusive ownership

    void lock()
    {
      unique_lock<mutex> lock(this->mutex_);
      while (write_entered_ & this->state_){
        this->gate1_.wait(lock);
      }
      
      this->state_ |= write_entered_;
      while (n_readers_ & this->state_){
        gate2_.wait(lock);
      }

#ifndef _WIN32
      this->owner_id_ = syscall(SYS_gettid);
#else
      this->owner_id_ = GetCurrentThreadId();
#endif
    }
    
    bool try_lock()
    {
      unique_lock<mutex> lock(this->mutex_, try_to_lock);
      
      if (lock.owns_lock() && this->state_ == 0) {
        this->state_ = write_entered_;
#ifndef _WIN32
        this->owner_id_ = syscall(SYS_gettid);
#else
        this->owner_id_ = GetCurrentThreadId();
#endif
        return true;
      }
      
      return false;
    }
    
    void unlock()
    {
      {
        lock_guard<mutex> lock(this->mutex_);
        this->state_ = 0;
        this->owner_id_ = 0;
      }
      this->gate1_.notify_all();
    }

    // Shared ownership

    void lock_shared()
    {
      unique_lock<mutex> lock(this->mutex_);
      while ((this->state_ & write_entered_) || (this->state_ & n_readers_) == n_readers_) {
        this->gate1_.wait(lock);
      }
      unsigned num_readers = (this->state_ & n_readers_) + 1;
      this->state_ &= ~n_readers_;
      this->state_ |= num_readers;
    }
    
    bool try_lock_shared()
    {
      unique_lock<mutex> lock(this->mutex_, try_to_lock);
      unsigned num_readers = this->state_ & n_readers_;
      if (lock.owns_lock() && !(this->state_ & write_entered_) && num_readers != n_readers_) {
        ++num_readers;
        this->state_ &= ~n_readers_;
        this->state_ |= num_readers;
        return true;
      }
      return false;
    }
    
    void unlock_shared()
    {
      lock_guard<mutex> lock(this->mutex_);
      unsigned num_readers = (this->state_ & n_readers_) - 1;
      this->state_ &= ~n_readers_;
      this->state_ |= num_readers;
      if (this->state_ & write_entered_) {
        if (num_readers == 0){
          this->gate2_.notify_one();
        }
      }
      else {
        if (num_readers == n_readers_ - 1) {
          this->gate1_.notify_one();
        }
      }
    }

  private:
    mutex    mutex_;
    condition_variable gate1_;
    condition_variable gate2_;
    unsigned state_;
#ifndef _WIN32
    pid_t owner_id_;
#else
    DWORD owner_id_;
#endif//_WIN32

    static const unsigned write_entered_ = 1U << (sizeof(unsigned)*8 - 1);
    static const unsigned n_readers_ = ~write_entered_;
  };
  
  template<typename mutex_type>
  class shared_lock
  {
  public:
    shared_lock(mutex_type & owner)
    : is_locked_(true), owner_(owner)
    {
      this->owner_.lock_shared();
    }
    
    ~shared_lock()
    {
      if(this->is_locked_) {
        this->owner_.unlock_shared();
      }
    }

    void unlock(){
      if(this->is_locked_) {
        this->owner_.unlock_shared();
        this->is_locked_ = false;
      }
    }

    void lock(){
      if(!this->is_locked_) {
        this->owner_.lock_shared();
        this->is_locked_ = true;
      }
    }

  private:
    bool is_locked_;
    mutex_type & owner_;
  };
}
#else

#include <shared_mutex>

#endif

#endif//__VDS_CORE_LEGACY_H_
