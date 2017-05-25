#ifndef __VDS_CORE_NOT_MUTEX_H_
#define __VDS_CORE_NOT_MUTEX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <mutex>

namespace vds {
  class not_mutex
  {
  public:

    void lock()
    {
      if (!this->m_.try_lock()) {
        throw std::runtime_error("Multithreading error");
      }
    }

    void unlock()
    {
      this->m_.unlock();
    }

  private:
    std::mutex m_;
  };
}



#endif//__VDS_CORE_NOT_MUTEX_H_
