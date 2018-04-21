/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <algorithm>
#include "task_manager.h"
#include "mt_service.h"
#include "shutdown_event.h"
#include "barrier.h"
#include "logger.h"

vds::timer::timer(const char * name)
: name_(name), sp_(service_provider::empty()),
current_state_(state_t::bof),
  is_shuting_down_(false)
{
}


vds::task_manager::task_manager()
: sp_(service_provider::empty()),
  is_shuting_down_(false)
{
}

vds::task_manager::~task_manager()
{
}


void vds::timer::start(
  const vds::service_provider& sp,
  const std::chrono::steady_clock::duration & period,
  const std::function< bool(void) >& callback)
{
  this->current_state_.change_state(state_t::bof, state_t::scheduled).wait();
  this->sp_ = sp;
  this->period_ = period;
  this->handler_ = callback;
  
  this->schedule(sp);
}

void vds::timer::stop(const vds::service_provider& sp)
{
	auto manager = static_cast<task_manager *>(sp.get<itask_manager>());

	std::lock_guard<std::mutex> lock(manager->scheduled_mutex_);
	manager->scheduled_.remove(this);
  this->is_shuting_down_ = true;
	this->current_state_.wait(state_t::eof).wait();
}

void vds::timer::execute(const vds::service_provider& sp)
{
  try {
    if (!this->is_shuting_down_ && !sp.get_shutdown_event().is_shuting_down()) {
      this->current_state_.change_state(state_t::scheduled, state_t::in_handler).wait();
      if (this->handler_()) {
        this->current_state_.change_state(state_t::in_handler, state_t::scheduled).wait();
        this->schedule(sp);
      } else {
        sp.get<logger>()->trace("tm", sp, "Task %s finished", this->name_.c_str());
        this->current_state_.change_state(state_t::in_handler, state_t::eof).wait();
      }
    } else {
      this->current_state_.change_state(state_t::scheduled, state_t::eof);
      sp.get<logger>()->trace("tm", sp, "Task finished by shuting down");
    }
  }
  catch (const std::exception & ex) {
    sp.unhandled_exception(std::make_shared<std::runtime_error>(ex.what()));
  }
  catch (...) {
    sp.unhandled_exception(std::make_shared<std::runtime_error>("Unexpected error"));
  }
}

void vds::timer::schedule(const vds::service_provider& sp)
{
  if(sp.get_shutdown_event().is_shuting_down()){
    return;
  }
  
  auto manager = static_cast<task_manager *>(sp.get<itask_manager>());
  
  this->start_time_ = std::chrono::steady_clock::now() + this->period_;

  std::lock_guard<std::mutex> lock(manager->scheduled_mutex_);
  manager->scheduled_.push_back(this);
  sp.get<logger>()->trace("tm", sp, "Add Task %s", this->name_.c_str());

  if (!manager->work_thread_.joinable()) {
    if (!manager->sp_) {
      throw std::runtime_error("Invalid task_manager state");
    }
    manager->work_thread_ = std::thread([manager]() {
      manager->work_thread();
    });
  }
}



void vds::task_manager::register_services(service_registrator & registrator)
{
  registrator.add_service<itask_manager>(this);
}

void vds::task_manager::start(const service_provider & sp)
{
  this->sp_ = sp.create_scope("task_manager");
  imt_service::enable_async(this->sp_);
}

void vds::task_manager::stop(const service_provider & sp)
{
  sp.get<logger>()->debug("tm", sp, "Stopping task manager");

  this->is_shuting_down_ = true;
  if (this->work_thread_.joinable()) {
    this->work_thread_.join();
  }
}

void vds::task_manager::work_thread()
{
  barrier b(0);
  
  while(!this->is_shuting_down_){
  
    std::unique_lock<std::mutex> lock(this->scheduled_mutex_);
    
    std::chrono::steady_clock::duration timeout = std::chrono::seconds(5);
    auto now = std::chrono::steady_clock::now();
    for(auto task : this->scheduled_){
      if(task->start_time_ <= now){
        this->scheduled_.remove(task);
        this->sp_.get<logger>()->trace("tm", this->sp_, "Remove Task %s", task->name_.c_str());

        ++b;
        
        imt_service::async(this->sp_, [this, task, &b](){
          try{
            task->execute(this->sp_);
            
            --b;
          }
          catch(...){
            throw;
          }
        });
        break;
      }
      else {
        auto delta = task->start_time_ - now;
        if (timeout > delta) {
          timeout = delta;
        }
      }
    }
    
    this->scheduled_changed_.wait_for(lock, timeout);
  }
  
  this->sp_.get<logger>()->debug("tm", this->sp_, "Waiting stop task manager");
  b.wait();
  this->sp_.get<logger>()->debug("tm", this->sp_, "Task manager stopped");
  
}
