//#ifndef __VDS_CORE_PARALLEL_TASKS_H_
//#define __VDS_CORE_PARALLEL_TASKS_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//
//
//namespace vds {
//
//  class parallel_tasks
//  {
//  public:
//
//    template <typename task_type>
//    void add(task_type && task)
//    {
//      this->tasks_.push_back(std::forward<task_type>(task));
//    }
//
//    vds::async_task<void> run()
//    {
//      return [this](const vds::async_result<> & result) {
//          auto runner = new _async_series(result, this->tasks_.size());
//          runner->run(std::move(this->tasks_));
//        };
//    }
//
//  private:
//    std::list<vds::async_task<void>> tasks_;
//  };
//}
//
//#endif // __VDS_CORE_PARALLEL_TASKS_H_
// 
