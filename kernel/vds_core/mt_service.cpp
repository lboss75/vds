/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "mt_service.h"
#include "shutdown_event.h"
#include "barrier.h"

#include "private/mt_service_p.h"

#include <thread>

#ifndef _WIN32
#include <sys/syscall.h>
#include <sys/types.h>
#endif//_WIN32

vds::mt_service::mt_service()
{
}

vds::mt_service::~mt_service()
{
}

void vds::mt_service::register_services(vds::service_registrator& registrator)
{
  registrator.add_service<imt_service>(this);
}

void vds::mt_service::start(const vds::service_provider& sp)
{
  this->impl_.reset(new _mt_service(sp));
  this->impl_->start();
}

void vds::mt_service::stop(const vds::service_provider&)
{
	if (this->impl_) {
		this->impl_->stop();
	}
}

void vds::imt_service::async(const std::function<void(void)>& handler)
{
  ((mt_service *)this)->impl_->async(handler);
}

void vds::imt_service::async(std::function<void(void)> && handler)
{
  ((mt_service *)this)->impl_->async(std::move(handler));
}

void vds::imt_service::enable_async(const service_provider & sp)
{
  sp.set_property<async_enabled_property>(service_provider::property_scope::any_scope, new async_enabled_property(true));
}

void vds::imt_service::disable_async(const service_provider & sp)
{
  sp.set_property<async_enabled_property>(service_provider::property_scope::any_scope, new async_enabled_property(false));
}

void vds::imt_service::async_enabled_check(const service_provider & sp)
{
  auto p = sp.get_property<async_enabled_property>(service_provider::property_scope::any_scope);
  if (nullptr != p && p->is_enabled_){
    return;
  }

  throw std::runtime_error("Async call is not enabled");
;}

vds::_mt_service::_mt_service(const service_provider & sp)
: sp_(sp), is_shuting_down_(false)
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
  this->is_shuting_down_ = true;
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

void vds::_mt_service::async(std::function<void(void)> && handler)
{
  std::unique_lock<std::mutex> lock(this->mutex_);
  this->queue_.push(std::move(handler));
  this->cond_.notify_all();
}

void vds::_mt_service::work_thread()
{
#ifndef _WIN32
  auto thread_id = syscall(SYS_gettid);
#endif

  while(!this->is_shuting_down_){
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

