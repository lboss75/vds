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
#include "logger.h"

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

void vds::mt_service::start(const service_provider * sp)
{
  this->impl_.reset(new _mt_service(sp));
  this->impl_->start();
}

void vds::mt_service::stop()
{
	if (this->impl_) {
		this->impl_->stop();
	}
}

std::future<void> vds::mt_service::prepare_to_stop() {
  return this->impl_->prepare_to_stop();
}

void vds::imt_service::do_async(const std::function<void(void)>& handler)
{
  ((mt_service *)this)->impl_->do_async(handler);
}

void vds::imt_service::do_async( std::function<void(void)> && handler)
{
  ((mt_service *)this)->impl_->do_async(std::move(handler));
}

vds::_mt_service::_mt_service(const service_provider * sp)
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
  for(auto & t : this->work_threads_){
    this->cond_.notify_all();
    t.join();
  }
}

std::future<void> vds::_mt_service::prepare_to_stop() {
  this->is_shuting_down_ = true;
  co_return;
}

void vds::_mt_service::do_async( const std::function<void(void)> & handler)
{
  std::unique_lock<std::mutex> lock(this->mutex_);
#if defined(DEBUG)
  this->queue_.push([sp = this->sp_, handler, thread_id =
#ifndef _WIN32
    syscall(SYS_gettid)
#else
    GetCurrentThreadId()
#endif
  ]() {
    sp->get<logger>()->trace("Async", "Anync from %d", thread_id);
    handler();
  });
#else//defined(DEBUG)
  this->queue_.push(handler);
#endif//defined(DEBUG)
  this->cond_.notify_all();
}

void vds::_mt_service::do_async( std::function<void(void)> && handler)
{
  std::unique_lock<std::mutex> lock(this->mutex_);
#if defined(DEBUG)
  this->queue_.push([sp = this->sp_, h = std::move(handler), thread_id =
#ifndef _WIN32
    syscall(SYS_gettid)
#else
    GetCurrentThreadId()
#endif
  ]() {
    sp->get<logger>()->trace("Async", "Anync from %d", thread_id);
    h();
  });
#else//defined(DEBUG)
  this->queue_.push(std::move(handler));
#endif//defined(DEBUG)
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
      this->cond_.wait(
        lock,
        [this]()->bool { return this->is_shuting_down_ || !this->queue_.empty(); });
      
      if(this->queue_.empty()){
        continue;
      }
      
      handler = this->queue_.front();
      this->queue_.pop();
    }    
    
    handler();
  }
}

