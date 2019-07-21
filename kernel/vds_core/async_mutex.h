#ifndef __VDS_CORE_ASYNC_MUTEX_H_
#define __VDS_CORE_ASYNC_MUTEX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"

namespace vds {
  class async_mutex {
  public:
    async_task<void> lock() {
      async_result<void> result;

      std::unique_lock<std::mutex> lock(this->task_mutex_);
      auto is_empty = this->tasks_.empty();
      this->tasks_.push_back(result);
      lock.unlock();
      
      if (is_empty) {
        result.set_value();
      }

      return result.get_future();
    }

    void unlock()
    {
      std::unique_lock<std::mutex> lock(this->task_mutex_);
      this->tasks_.pop_front();
      if (!this->tasks_.empty()) {
        this->tasks_.front().set_value();
      }
      lock.unlock();
    }

  private:
    std::mutex task_mutex_;
    std::list<async_result<void>> tasks_;
  };
}

#endif // __VDS_CORE_ASYNC_MUTEX_H_
 
