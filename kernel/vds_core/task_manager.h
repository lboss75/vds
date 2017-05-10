#ifndef __VDS_CORE_TASK_MANAGER_H_
#define __VDS_CORE_TASK_MANAGER_H_

#include <chrono>
#include <list>
#include <condition_variable>
#include <thread>

#include "service_provider.h"
#include "debug.h"

namespace vds {
  
  class timer
  {
  public:
    timer();
    
    void start(
      const service_provider & sp,
      const std::chrono::steady_clock::duration & period,
      const std::function<bool(void)> & callback);
    
    void stop(const vds::service_provider& sp);
    
  private:
    friend class task_manager;
    service_provider sp_;
    std::chrono::steady_clock::duration period_;
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
    std::function<bool(void)> handler_;
    
    void execute(const service_provider& sp);
    void schedule(const service_provider& sp);
  };

  class itask_manager
  {
  public:
  };

  class task_manager : public iservice_factory, public itask_manager
  {
  public:
    task_manager();
    ~task_manager();

    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
      
  private:
    friend class timer;
   
    service_provider sp_;
    std::list<timer *> scheduled_;
    std::condition_variable scheduled_changed_;
    std::mutex scheduled_mutex_;
    std::thread work_thread_;

    void work_thread();
  };
}

#endif // __VDS_CORE_TASK_MANAGER_H_
