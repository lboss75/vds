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
#include "async_task.h"

vds::timer::timer(const char * name)
: name_(name),
  current_state_(std::make_shared<state_machine<state_t>>(state_t::bof)),
  is_shuting_down_(false)
{
}

vds::timer::~timer() {
  vds_assert(!this->is_started());
}


vds::task_manager::task_manager()
: is_shuting_down_(false), is_disabled_(false)
{
}

vds::task_manager::~task_manager()
{
}

bool vds::timer::is_started() const
{
  return (this->current_state_->state() != state_t::bof);
}

vds::expected<void> vds::timer::start(
  const service_provider * sp,
  const std::chrono::steady_clock::duration & period,
  const std::function<async_task<expected<bool>>(void) >& callback)
{
  this->sp_ = sp;
  CHECK_EXPECTED(this->current_state_->change_state(state_t::bof, state_t::scheduled).get());
  this->period_ = period;
  this->handler_ = callback;
  
  this->schedule();

  return expected<void>();
}

vds::expected<void> vds::timer::stop()
{
	auto manager = static_cast<task_manager *>(this->sp_->get<task_manager>());

	std::lock_guard<std::mutex> lock(manager->scheduled_mutex_);
	manager->scheduled_.remove(this);
  this->is_shuting_down_ = true;
  CHECK_EXPECTED(this->current_state_->wait(state_t::eof).get());

  return expected<void>();
}

void vds::timer::execute()
{
    auto r = this->execute_async().get();
    if(r.has_error()) {
      this->sp_->get<logger>()->warning("tm", "Timer execute error %s", r.error()->what());
    }
}

void vds::timer::schedule()
{
  if(this->sp_->get_shutdown_event().is_shuting_down()){
    return;
  }
  
  auto manager = static_cast<task_manager *>(this->sp_->get<task_manager>());

  if(manager->is_disabled_) {
    return;
  }
  
  this->start_time_ = std::chrono::steady_clock::now() + this->period_;

  std::lock_guard<std::mutex> lock(manager->scheduled_mutex_);
  manager->scheduled_.push_back(this);
  this->sp_->get<logger>()->trace("tm", "Add Task %s", this->name_.c_str());

  if (!manager->work_thread_.joinable()) {
    manager->work_thread_ = std::thread([manager]() {
      manager->work_thread();
    });
  }
}

vds::async_task<vds::expected<void>> vds::timer::execute_async() {
  if (!this->is_shuting_down_) {
    CHECK_EXPECTED_ASYNC(co_await this->current_state_->change_state(state_t::scheduled, state_t::in_handler));

    auto r = co_await this->handler_();
    if (r.has_error()) {
      co_return vds::unexpected(std::move(r.error()));
    }

    if (r.value()) {
      CHECK_EXPECTED_ASYNC(co_await this->current_state_->change_state(state_t::in_handler, state_t::scheduled));
      this->schedule();
    }
    else {
      CHECK_EXPECTED_ASYNC(co_await this->current_state_->change_state(state_t::in_handler, state_t::eof));
    }
  }
  else {
    CHECK_EXPECTED_ASYNC(co_await this->current_state_->change_state(state_t::scheduled, state_t::eof));
  }
  co_return vds::expected<void>();
}


vds::expected<void> vds::task_manager::register_services(service_registrator & registrator)
{
  registrator.add_service<task_manager>(this);
  return expected<void>();
}

vds::expected<void> vds::task_manager::start(const service_provider * sp)
{
  this->sp_ = sp;
  return expected<void>();
}

vds::expected<void> vds::task_manager::stop()
{
  this->sp_->get<logger>()->debug("tm", "Stopping task manager");

  if (this->work_thread_.joinable()) {
    this->work_thread_.join();
  }

  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::task_manager::prepare_to_stop() {
  this->is_shuting_down_ = true;
  this->scheduled_changed_.notify_all();
  co_return expected<void>();
}

class barrier_locker
{
public:
  barrier_locker(vds::barrier * b)
  : b_(b) {
    ++*this->b_;
  }

  barrier_locker(barrier_locker && original)
    : b_(original.b_) {
    original.b_ = nullptr;
  }

  barrier_locker(const barrier_locker &) = delete;
  barrier_locker& operator = (const barrier_locker &) = delete;

  ~barrier_locker() {
    if (nullptr != this->b_) {
      --*this->b_;
    }
  }
private:
  vds::barrier * b_;
};

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
        
        imt_service::async(this->sp_, [task, l = barrier_locker(&b)](){
          task->execute();
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
  
  b.wait();
  
}
