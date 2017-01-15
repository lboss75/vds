/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "mt_service.h"
#include "shutdown_exception.h"
#include "logger.h"
#include "async_task.h"

vds::mt_service::mt_service()
{
}

vds::mt_service::~mt_service()
{
}

void vds::mt_service::register_services(service_registrator & registrator)
{
  registrator.add_factory<imt_service>(
    [this](bool & is_scoped)-> imt_service {
      is_scoped = false;
      return imt_service(this);
  });
  
  for (unsigned int i = 0; i < 2 * std::thread::hardware_concurrency(); ++i) {
    this->work_threads_.push_back(
      new std::thread(&vds::mt_service::work_thread, this));
  }
}

void vds::mt_service::start(const service_provider &)
{
}

void vds::mt_service::stop(const service_provider & sp)
{
    try {
        //Send end message
        this->async_tasks_lock_.lock();
        this->async_tasks_.push(nullptr);
        this->have_async_tasks_.notify_all();
        this->async_tasks_lock_.unlock();

        //Wait comlete
        for (auto w : this->work_threads_) {
            w->join();
        }

        this->async_tasks_lock_.lock();
        while (!this->async_tasks_.empty()) {
            auto task = this->async_tasks_.front();
            this->async_tasks_.pop();

            if (nullptr != task) {
                //task->on_shutdown();
            }
        }
        this->async_tasks_lock_.unlock();
    }
    catch (const std::exception * ex) {
        delete ex;
    }
    catch (...) {
    }
}

void vds::mt_service::work_thread()
{
  for (;;) {
    std::unique_lock<std::mutex> locker(this->async_tasks_lock_);
    this->have_async_tasks_.wait(locker, [this]()->bool { return !this->async_tasks_.empty(); });

    if (this->async_tasks_.empty()) {
      continue;
    }

    auto task = this->async_tasks_.front();
    if (!task) {
      break;
    }

    this->async_tasks_.pop();
    locker.unlock();

    task->execute();
  }
}

void vds::mt_service::schedule(const vds::async_task_base* task)
{
  std::unique_lock<std::mutex> locker(this->async_tasks_lock_);
  if (this->async_tasks_.empty()) {
      this->have_async_tasks_.notify_all();
  }
  
  this->async_tasks_.push(task);
}
