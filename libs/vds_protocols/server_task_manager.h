#ifndef __VDS_PROTOCOLS_SERVER_TASK_MANAGER_H_
#define __VDS_PROTOCOLS_SERVER_TASK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _server_task_manager;
  
  class server_task_manager
  {
  public:
    server_task_manager();
    ~server_task_manager();
    
    void start(const service_provider & sp);
    void stop(const service_provider & sp);

  private:
    _server_task_manager * const impl_;
  };
}

#endif // __VDS_PROTOCOLS_SERVER_TASK_MANAGER_H_
