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
    
    auto now = std::chrono::system_clock::now();
    for(auto task : this->scheduled_){
      if(task->start_time() <= now){
        this->scheduled_.remove(task);
        this->sp_.get<imt_service>().async([task](){
          task->execute();
        });
        break;
      }
    }
    
    this->scheduled_changed_.wait_for(lock, std::chrono::seconds(1));
  }
}

void vds::task_manager::task_job_base::start()
{
  this->schedule(std::chrono::system_clock::now());
}

void vds::task_manager::task_job_base::schedule(const std::chrono::time_point<std::chrono::system_clock>& start)
{
  this->start_time_ = start;

  std::unique_lock<std::mutex> lock(this->owner_->scheduled_mutex_);
  this->owner_->scheduled_.push_back(this);

  if (!this->owner_->work_thread_.valid()) {
    this->owner_->work_thread_ = std::async(std::launch::async, [this]() {
      this->owner_->work_thread();
    });
  }
}

void vds::task_manager::task_job_base::destroy()
{
  std::unique_lock<std::mutex> lock(this->owner_->scheduled_mutex_);
  this->owner_->scheduled_.remove(this);
}
