#ifndef __VDS_CORE_TASK_MANAGER_H_
#define __VDS_CORE_TASK_MANAGER_H_

#include "service_provider.h"

namespace vds {
 
  class task_manager : public iservice
  {
  public:
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
      
  private:
    friend class itask_manager;
    friend class task_job;
    
    class task_job_base
    {
    public:
      task_job_base(task_manager * owner)
      : owner_(owner)
      {
      }
      
      virtual ~task_job_base()
      {
      }
      
      virtual void execute() = 0;
      
      void start();
      void schedule(const std::chrono::time_point<std::chrono::system_clock> & start);
      void destroy();
      
      const std::chrono::time_point<std::chrono::system_clock> & start_time() const
      {
        return this->start_time_;
      }
      
    private:
      std::chrono::time_point<std::chrono::system_clock> start_time_;
      task_manager * owner_;      
    };
    
    std::list<task_job_base *> scheduled_;
    std::condition_variable scheduled_changed_;
    std::mutex scheduled_mutex_;
    void work_thread();
    
    template <typename handler_type>
    class task_job_impl : public task_job_base
    {
    public:
      task_job_impl(task_manager * owner, handler_type & handler)
      : task_job_base(owner), handler_(handler)
      {
      }
      
      void execute() override
      {
        this->handler_();
      }
      
    private:
      handler_type & handler_;
    };
  };
  
  class task_job
  {
  public:
    
    void start()
    {
      this->impl_->start();
    }
    
    void schedule(const std::chrono::time_point<std::chrono::system_clock> & start)
    {
      this->impl_->schedule(start);
    }
    
    void destroy()
    {
      this->impl_->destroy();
    }
    
  private:
    friend class itask_manager;
    task_manager::task_job_base * impl_;
    
    task_job(task_manager::task_job_base * impl)
    : impl_(impl)
    {
    }
  };
  
  class itask_manager
  {
  public:
    itask_manager(task_manager * owner)
    : owner_(owner)
    {
    }
    
    template <typename handler_type>
    task_job create_job(handler_type & handler)
    {
      return task_job(new task_manager::task_job_impl<handler_type>(this->owner_, handler));
    }
    
  private:
    task_manager * owner_;
  };

}

#endif // __VDS_CORE_TASK_MANAGER_H_
