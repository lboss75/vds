#ifndef __VDS_CORE_THREAD_APARTMENT_H_
#define __VDS_CORE_THREAD_APARTMENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <queue>
#include <functional>
#include <mutex>

namespace vds {
  class thread_apartment {
  public:
    void schedule(const std::function<void(void)> & callback);

  private:
    std::mutex task_queue_mutex_;
    std::queue<std::function<void(void)>> task_queue_;

  };
}

#endif //__VDS_CORE_THREAD_APARTMENT_H_
