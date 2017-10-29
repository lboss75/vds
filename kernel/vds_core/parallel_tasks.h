#ifndef __VDS_CORE_PARALLEL_TASKS_H_
#define __VDS_CORE_PARALLEL_TASKS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"

namespace vds {

  class parallel_tasks
  {
  public:

    template <typename task_type>
    void add(task_type && task)
    {
      this->tasks_.push_back(std::forward<task_type>(task));
    }

    async_task<> run()
    {
      return [this](const async_result<> & result) {
          auto runner = new _async_series(result, this->tasks_.size());
          runner->run(std::move(this->tasks_));
        };
    }

  private:
    std::list<async_task<>> tasks_;
  };
}

#endif // __VDS_CORE_PARALLEL_TASKS_H_
 
