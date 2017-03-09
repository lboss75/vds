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
    friend class task_job;
    
    class task_job_base
    {
    public:
      task_job_base(const std::string & name, task_manager * owner)
      : name_(name), owner_(owner)
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
      std::string name_;
      task_manager * owner_;      
    };
    
    service_provider sp_;
    std::list<task_job_base *> scheduled_;
    std::condition_variable scheduled_changed_;
    std::mutex scheduled_mutex_;
    std::future<void> work_thread_;

    void work_thread();
    
    
    template <typename handler_type>
    class task_job_impl : public task_job_base
    {
    public:
      task_job_impl(const std::string & name, task_manager * owner, handler_type & handler)
      : task_job_base(name, owner), handler_(handler)
      {
      }
      
      void execute() override
      {
        this->handler_();
      }
      
    private:
      lifetime_check ft_;
      handler_type & handler_;
    };

    template <typename class_name>
    class task_job_class_impl : public task_job_base
    {
    public:
      task_job_class_impl(const std::string & name, task_manager * owner, class_name * handler_class, void (class_name::*handler)())
        : task_job_base(name, owner), handler_class_(handler_class), handler_(handler)
      {
      }

      void execute() override
      {
        (this->handler_class_->*handler_)();
      }

    private:
      lifetime_check ft_;
      class_name * handler_class_;
      void (class_name::*handler_)();
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
    
    static itask_manager get(const service_provider & sp)
    {
      return sp.get<itask_manager>();
    }
    
    template<typename _Rep, typename _Period>
    void wait_for(
      const std::chrono::duration<_Rep, _Period> & period,
      event_handler<> & timeout_handler)
    {
    }
    
    template<typename _Rep, typename _Period, typename... event_data_types>
    void wait_for(
      const std::chrono::duration<_Rep, _Period> & period,
      event_source<event_data_types...> & source,
      event_handler<event_data_types...> & handler,
      event_handler<> & timeout_handler)
    {
      auto proxy = new handler_proxy(handler, timeout_handler);
      source += *proxy;
      this->wait_for<_Rep, _Period>(period, *proxy);      
    }

  private:
    task_manager * owner_;
    
    template<typename... event_data_types>
    class handler_proxy
    {
    public:
      void operator()(event_data_types... args)
      {
      }
    };
    
  };

}

#endif // __VDS_CORE_TASK_MANAGER_H_
