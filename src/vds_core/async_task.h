#ifndef __VDS_CORE_ASYNC_TASK_H_
#define __VDS_CORE_ASYNC_TASK_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class service_provider;
  
  class async_task_base
  {
  public:
    virtual ~async_task_base();
    virtual void execute() const = 0;
  };
  
  template <
    typename done_method_type,
    typename error_handler_type
  >
  class async_task : public async_task_base
  {
  public:
    async_task(
      const done_method_type & done,
      const error_handler_type & on_error
    ) : done_(done), on_error_(on_error)
    {
    }
    
    void schedule(service_provider & sp) const
    {
      sp.get<imt_service>().schedule(this);
    }
    
    void execute() const override
    {
      try {
        this->done_();
      }
      catch(std::exception * ex) {
        this->on_error_(ex);
      }
    }
    
  private:
    const done_method_type & done_;
    const error_handler_type & on_error_;
  };
}

#endif // __VDS_CORE_ASYNC_TASK_H_
