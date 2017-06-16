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

    void add(const async_task<> & task)
    {
      this->tasks_.push_back(task);
    }

    async_task<> run()
    {
      return create_async_task(
        [this](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
          auto runner = new _async_series(done, on_error, sp, this->tasks_.size());
          runner->run(this->tasks_);
        });
    }

  private:
    std::list<async_task<>> tasks_;
  };
}

#endif // __VDS_CORE_PARALLEL_TASKS_H_
 