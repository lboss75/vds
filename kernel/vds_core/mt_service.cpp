/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "mt_service.h"
#include "mt_service_p.h"

vds::mt_service::mt_service()
{
}

vds::mt_service::~mt_service()
{
}

void vds::mt_service::register_services(vds::service_registrator& registrator)
{
  registrator.add_factory<imt_service>([this](const service_provider & /*sp*/, bool &)->imt_service{
    return imt_service(this);
  });
}

void vds::mt_service::start(const vds::service_provider& sp)
{
  this->impl_.reset(new _mt_service(sp));
  this->impl_->start();
}

void vds::mt_service::stop(const vds::service_provider&)
{
  this->impl_->stop();
}

void vds::imt_service::async(const std::function<void(void)>& handler)
{
  this->owner_->impl_->async(handler);
}

vds::_mt_service::_mt_service(const service_provider & sp)
: sp_(sp)
{
}

void vds::_mt_service::start()
{
  unsigned int count = std::thread::hardware_concurrency();
  if(count < 1){
    count = 1;
  }
  else if(count > 1024 * 1024) {
    count = 1024 * 1024;
  }
  
  for(unsigned int i = 0; i < count; ++i){
    this->work_threads_.push_back(std::thread(std::bind(&_mt_service::work_thread, this)));
  }
}

void vds::_mt_service::stop()
{
  for(auto & t : this->work_threads_){
    this->cond_.notify_all();
    t.join();
  }
}

void vds::_mt_service::async(const std::function<void(void)> & handler)
{
  std::unique_lock<std::mutex> lock(this->mutex_);
  this->queue_.push(handler);
  this->cond_.notify_all();
}

void vds::_mt_service::work_thread()
{
  while(!this->sp_.get_shutdown_event().is_shuting_down()){
    std::function<void(void)> handler;
    {
      std::unique_lock<std::mutex> lock(this->mutex_);
      this->cond_.wait_for(
        lock,
        std::chrono::seconds(1),
        [this]()->bool { return !this->queue_.empty(); });
      
      if(this->queue_.empty()){
        continue;
      }
      
      handler = this->queue_.front();
      this->queue_.pop();
    }    
    
    handler();
  }
}

