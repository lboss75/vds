/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "thread_apartment.h"

void vds::thread_apartment::schedule(const std::function<void(void)> &callback) {
  std::unique_lock<std::mutex> lock(this->task_queue_mutex_);
  bool need_start = this->task_queue_.empty();
  this->task_queue_.emplace(callback);

  if(need_start){
    for(;;) {
      auto & head = this->task_queue_.front();
      lock.unlock();

      head();

      lock.lock();
      this->task_queue_.pop();
      if(this->task_queue_.empty()){
        break;
      }
    }
  }
}
