/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "task_manager.h"

void vds::task_manager::work_thread()
{
  std::unique_lock<std::mutex> lock(this->scheduled_mutex_);
  
  std::chrono::time_point<std::chrono::system_clock> start;
  for(auto task : this->scheduled_){
    if(start > task->start_time()){
      start = task->start_time();
    }
  }

  this->scheduled_changed_.wait_until(lock, start);
}

