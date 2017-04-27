#ifndef __VDS_CORE_TASK_MANAGER_H_
#define __VDS_CORE_TASK_MANAGER_H_

#include "service_provider.h"
#include "debug.h"
#include "events.h"

namespace vds {

  class itask_manager
  {
  public:
    static itask_manager get(const service_provider & sp)
    {
      return sp.get<itask_manager>();
    }

    event_source<> & wait_for(const std::chrono::steady_clock::duration & period);
    event_source<> & schedule(const std::chrono::time_point<std::chrono::steady_clock> & start);

    void wait_for(const std::chrono::steady_clock::duration & period, const std::function<void(void)> & callback);
  };

  class task_manager : public iservice_factory, public itask_manager
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
      task_job(
        task_manager * owner,
        const std::function<void(void)> & handler)
      : owner_(owner), handler_(handler)
      {
      }
      task_job(
        task_manager * owner)
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
      
      void operator()()
      {
        if(this->handler_){
          this->handler_();
        }
        else {
          event_source<>::operator()();
        }
      }
      
    private:
      std::chrono::time_point<std::chrono::steady_clock> start_time_;
      task_manager * owner_;
      std::function<void(void)> handler_;
    };
    
    service_provider sp_;
    std::list<task_job *> scheduled_;
    std::condition_variable scheduled_changed_;
    std::mutex scheduled_mutex_;
    std::thread work_thread_;

    void work_thread();
  };
  
 

}

#endif // __VDS_CORE_TASK_MANAGER_H_
