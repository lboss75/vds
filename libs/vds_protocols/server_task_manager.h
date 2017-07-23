#ifndef __VDS_PROTOCOLS_SERVER_TASK_MANAGER_H_
#define __VDS_PROTOCOLS_SERVER_TASK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>
#include "service_provider.h"

namespace vds {
  class _server_task_manager;
  
  class server_task_manager
  {
  public:
    server_task_manager();
    ~server_task_manager();
    
    void start(const service_provider & sp);
    void stop(const service_provider & sp);

    enum class task_status
    {
      QUERED,
      IN_PROGRESS,
      PAUSED,
      FAILED,
      DONE
    };

    struct task_state
    {
      task_status status;
      std::string current_task;
      int progress_percent;
      std::chrono::system_clock::time_point start_time;
      std::chrono::system_clock::time_point finish_time;
    };

  private:
    _server_task_manager * const impl_;
  };
}

#endif // __VDS_PROTOCOLS_SERVER_TASK_MANAGER_H_
