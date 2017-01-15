/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifndef __VDS_CORE_MT_SERVICE_H_
#define __VDS_CORE_MT_SERVICE_H_

#include <list>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "service_provider.h"
#include "windows_event.h"
#include "func_utils.h"

namespace vds {
  class async_task_base;
  
  class mt_service : public iservice
  {
  public:
    mt_service();
    ~mt_service();

    // Inherited via iservice
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
    

  private:
    friend class imt_service;
    std::mutex async_tasks_lock_;
    std::condition_variable have_async_tasks_;
    std::queue<const async_task_base *> async_tasks_;

    std::list<std::thread *> work_threads_;
    void work_thread();
    
    void schedule(const async_task_base * task);
  };
  
  class imt_service
  {
  public:
    imt_service(mt_service * owner)
    : owner_(owner)
    {
    }
    
    void schedule(const async_task_base * task)
    {
      this->owner_->schedule(task);
    }
    
  private:
    mt_service * owner_;
  };
}

#endif // !__VDS_CORE_MT_SERVICE_H_
