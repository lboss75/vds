/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifndef __VDS_CORE_MT_SERVICE_H_
#define __VDS_CORE_MT_SERVICE_H_

#include <list>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "service_provider.h"
#include "windows_event.h"
#include "func_utils.h"

namespace vds {
    class mt_service;

    class imt_service
    {
    public:
    private:
        mt_service * owner_;

        friend class async_result;
        friend class mt_service;

        imt_service(mt_service * owner);
    };

    class mt_service : public iservice
    {
    public:
        mt_service();
        ~mt_service();

        // Inherited via iservice
        void register_services(service_registrator &) override;
        void start(const service_provider &) override;
        void stop(const service_provider &) override;

    private:
        class task_info : public std::enable_shared_from_this<task_info> {
        public:
            task_info(
                mt_service * owner,
                const std::function<void(void)> & task,
                const std::function<void(void)> & done,
                const error_handler_t & error_handler
                );
            ~task_info();

            void execute();
            void wait();
            void on_shutdown();

        private:
            mt_service * owner_;
            std::condition_variable comple_wait_;
            std::function<void(void)> task_;
            std::function<void(void)> done_;
            error_handler_t error_handler_;
        };

        std::mutex async_tasks_lock_;
        std::condition_variable have_async_tasks_;
        std::queue<std::shared_ptr<task_info>> async_tasks_;
        friend class async_result;

        std::list<std::thread *> work_threads_;
        void work_thread();

        std::shared_ptr<task_info> exec_async(
            std::function<void(void)> task,
            std::function<void(void)> done,
            const error_handler_t & error_handler
            );
    };

    class async_result
    {
    public:
        void begin(
            const service_provider & sp,
            const std::function<void(void)> & task,
            const std::function<void(void)> & done,
            const error_handler_t & error_handler
            );
        void wait();

        bool operator ! () const {
            return !this->task_info_;
        }

    private:
        std::shared_ptr<mt_service::task_info> task_info_;

    };
}

#endif // !__VDS_CORE_MT_SERVICE_H_
