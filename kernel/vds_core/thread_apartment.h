#ifndef __VDS_CORE_THREAD_APARTMENT_H_
#define __VDS_CORE_THREAD_APARTMENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <queue>
#include <functional>
#include <mutex>
#include "mt_service.h"
#include "vds_debug.h"

namespace vds {

  class thread_apartment : public std::enable_shared_from_this<thread_apartment> {
  public:
    ~thread_apartment() {
      vds_assert(this->task_queue_.empty());
    }

    void schedule(const service_provider & sp, const std::function<void(void)> & callback) {
      std::unique_lock<std::mutex> lock(this->task_queue_mutex_);
      const auto need_start = this->task_queue_.empty();
      this->task_queue_.push(callback);
      lock.unlock();

      if(need_start) {
        mt_service::async(sp, [pthis = this->shared_from_this()]() {
          for (;;) {
            std::unique_lock<std::mutex> lock(pthis->task_queue_mutex_);
            auto f = pthis->task_queue_.front();
            lock.unlock();

            try {
              f();
            }
            catch (...) {
            }

            lock.lock();
            pthis->task_queue_.pop();
            if (pthis->task_queue_.empty()) {
              break;
            }
            lock.unlock();
          }
        });
      }
    }

  private:
    std::mutex task_queue_mutex_;
    std::queue<std::function<void(void)>> task_queue_;

  };
}

#endif //__VDS_CORE_THREAD_APARTMENT_H_
