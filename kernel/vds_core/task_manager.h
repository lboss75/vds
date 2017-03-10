#ifndef __VDS_CORE_TASK_MANAGER_H_
#define __VDS_CORE_TASK_MANAGER_H_

#include "service_provider.h"
#include "debug.h"
#include "events.h"

namespace vds {
 
  class task_manager : public iservice
  {
  public:
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
      
  private:
    friend class itask_manager;
    
    class task_job : public event_source<>
    {
    public:
      task_job(task_manager * owner)
      : owner_(owner)
      {
      }
      
      virtual ~task_job()
      {
      }
     
      void schedule(const std::chrono::time_point<std::chrono::steady_clock> & start);
      
      const std::chrono::time_point<std::chrono::steady_clock> & start_time() const
      {
        return this->start_time_;
      }
      
    private:
      std::chrono::time_point<std::chrono::steady_clock> start_time_;
      task_manager * owner_;      
    };
    
    service_provider sp_;
    std::list<task_job *> scheduled_;
    std::condition_variable scheduled_changed_;
    std::mutex scheduled_mutex_;
    std::future<void> work_thread_;

    void work_thread();
  };
  
 
  class itask_manager
  {
  public:
    itask_manager(task_manager * owner)
    : owner_(owner)
    {
    }
    
    static itask_manager get(const service_provider & sp)
    {
      return sp.get<itask_manager>();
    }
    
    event_source<> & wait_for(const std::chrono::steady_clock::duration & period);
    event_source<> & schedule(const std::chrono::time_point<std::chrono::steady_clock> & start);

  private:
    task_manager * owner_;
  };

}

#endif // __VDS_CORE_TASK_MANAGER_H_
