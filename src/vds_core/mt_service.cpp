/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "mt_service.h"
#include "shutdown_exception.h"
#include "logger.h"

vds::mt_service::mt_service()
{
}

vds::mt_service::~mt_service()
{
}

void vds::mt_service::register_services(service_registrator & registrator)
{
    registrator.add_factory<imt_service>([this](bool & is_scoped)->imt_service {
        is_scoped = true;
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
        this->async_tasks_.push(std::shared_ptr<task_info>());
        this->have_async_tasks_.notify_all();
        this->async_tasks_lock_.unlock();

        //Wait comlete
        for (auto w : this->work_threads_) {
            w->join();
        }

        this->async_tasks_lock_.lock();
        while (!this->async_tasks_.empty()) {
            std::shared_ptr<task_info> task = this->async_tasks_.front();
            this->async_tasks_.pop();

            if (nullptr != task) {
                task->on_shutdown();
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

        std::shared_ptr<task_info> task = this->async_tasks_.front();
        if (!task) {
            break;
        }

        this->async_tasks_.pop();
        locker.unlock();

        task->execute();
    }
}

std::shared_ptr<vds::mt_service::task_info>
vds::mt_service::exec_async(
    std::function<void(void)> task,
    std::function<void(void)> done,
    const error_handler_t & error_handler
    )
{
    std::shared_ptr<task_info> result(new task_info(this, task, done, error_handler));

    std::unique_lock<std::mutex> locker(this->async_tasks_lock_);
    if (this->async_tasks_.empty()) {
        this->have_async_tasks_.notify_all();
    }
    this->async_tasks_.push(result);

    return result;
}

vds::mt_service::task_info::task_info(
    mt_service * owner,
    const std::function<void(void)>& task,
    const std::function<void(void)> & done,
    const error_handler_t & error_handler
    )
    : owner_(owner), task_(task), done_(done), error_handler_(error_handler)
{
}

vds::mt_service::task_info::~task_info()
{
}


void vds::mt_service::task_info::execute()
{
    try {
        this->task_();
        this->done_();
    }
    catch (std::exception * ex) {
        this->error_handler_(ex);
    }

    this->comple_wait_.notify_all();
}

void vds::mt_service::task_info::wait()
{
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    this->comple_wait_.wait(lock);
}

void vds::mt_service::task_info::on_shutdown()
{
    this->done_();
}

void vds::async_result::begin(
    const service_provider & sp,
    const std::function<void(void)> & task,
    const std::function<void(void)> & done,
    const error_handler_t & error_handler
    )
{
    this->task_info_ = sp.get<imt_service>().owner_->exec_async(task, done, error_handler);
}

void vds::async_result::wait()
{
    this->task_info_->wait();
}

vds::imt_service::imt_service(mt_service * owner)
    : owner_(owner)
{
}
