/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "task_manager.h"
#include "mt_service.h"

void vds::task_manager::register_services(service_registrator & registrator)
{
  registrator.add_factory<itask_manager>(
    [this](const service_provider &, bool &)
    {
      return itask_manager(this);
  });
}

void vds::task_manager::start(const service_provider & sp)
{
  this->sp_ = sp;
}

void vds::task_manager::stop(const service_provider &)
{
  try {
    if (this->work_thread_.valid()) {
      this->work_thread_.get();
    }
  }
  catch (...) {
  }
}

void vds::task_manager::work_thread()
{
  while(!this->sp_.get_shutdown_event().is_shuting_down()){
  
    std::unique_lock<std::mutex> lock(this->scheduled_mutex_);
    
    std::chrono::steady_clock::duration timeout = std::chrono::seconds(5);
    auto now = std::chrono::steady_clock::now();
    for(auto task : this->scheduled_){
      if(task->start_time() <= now){
        this->scheduled_.remove(task);
        this->sp_.get<imt_service>().async([task](){
          (*task)();
          delete task;
        });
        break;
      }
      else {
        auto delta = task->start_time() - now;
        if (timeout > delta) {
          timeout = delta;
        }
      }
    }
    
    this->scheduled_changed_.wait_for(lock, timeout);
  }
}

void vds::task_manager::task_job::schedule(const std::chrono::time_point<std::chrono::steady_clock>& start)
{
  this->start_time_ = start;

  std::lock_guard<std::mutex> lock(this->owner_->scheduled_mutex_);
  this->owner_->scheduled_.push_back(this);

  if (!this->owner_->work_thread_.valid()) {
    this->owner_->work_thread_ = std::async(std::launch::async, [this]() {
      this->owner_->work_thread();
    });
  }
}

vds::event_source<>& vds::itask_manager::wait_for(const std::chrono::steady_clock::duration & period)
{
  return this->schedule(std::chrono::steady_clock::now() + period);
}

vds::event_source<>&  vds::itask_manager::schedule(const std::chrono::time_point<std::chrono::steady_clock>& start)
{
  auto result = new task_manager::task_job(this->owner_);
  result->schedule(start);
  return *result;
}

void vds::itask_manager::wait_for(
  const std::chrono::steady_clock::duration& period,
  const std::function<void(void)>& callback)
{
  auto result = new task_manager::task_job(this->owner_, callback);
  result->schedule(std::chrono::steady_clock::now() + period);
}
