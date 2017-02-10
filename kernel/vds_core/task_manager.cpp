/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "task_manager.h"

void vds::task_manager::register_services(service_registrator & registrator)
{
  registrator.add_factory<itask_manager>(
    [this](bool &)
    {
      return itask_manager(this);
  });
}

void vds::task_manager::start(const service_provider &)
{
}

void vds::task_manager::stop(const service_provider &)
{
}

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

void vds::task_manager::task_job_base::start()
{
  this->schedule(std::chrono::system_clock::now());
}

void vds::task_manager::task_job_base::schedule(const std::chrono::time_point<std::chrono::system_clock>& start)
{
  this->start_time_ = start;

  std::unique_lock<std::mutex> lock(this->owner_->scheduled_mutex_);

  auto need_execute = this->owner_->scheduled_.empty();
  this->owner_->scheduled_.push_back(this);

  if (need_execute) {
    std::async(std::bind(&task_manager::work_thread, this->owner_));
  }
}

void vds::task_manager::task_job_base::destroy()
{
  std::unique_lock<std::mutex> lock(this->owner_->scheduled_mutex_);
  this->owner_->scheduled_.remove(this);
}
