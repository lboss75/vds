#ifndef __VDS_NETWORK_NETWORK_SERVICE_P_H_
#define __VDS_NETWORK_NETWORK_SERVICE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <functional>
#include <vector>
#include <future>

#ifndef _WIN32
#include <sys/epoll.h>
#endif

#include "service_provider.h"
#include "network_service.h"
#include "network_types_p.h"
#include "task_manager.h"

namespace vds {
    class network_service;
    class socket_base;

    class _network_service : public std::enable_shared_from_this<_network_service>
    {
    public:
        _network_service();
        ~_network_service();

        // Inherited via iservice
        expected<void> start(const service_provider *);
        expected<void> stop();
        vds::async_task<vds::expected<void>> prepare_to_stop();
        
        void remove(socket_base * socket);

#ifdef _WIN32
        expected<void> associate(SOCKET_HANDLE s);

#else
        expected<void> associate(
          SOCKET_HANDLE s,
          const std::shared_ptr<socket_base> & handler,
          uint32_t event_mask);
        expected<void> set_events(
          SOCKET_HANDLE s,
          uint32_t event_mask);
        expected<void> remove_association(
          SOCKET_HANDLE s);
#endif
        
    private:
        friend class network_socket;
        friend class _udp_socket;
        friend class server_socket;
        friend class _read_socket_task;
        friend class _write_socket_task;
        
        const service_provider * sp_;
        std::mutex tasks_mutex_;
        std::condition_variable tasks_cond_;

#ifdef _WIN32
        HANDLE handle_;
        void thread_loop();
        std::list<std::thread *> work_threads_;
#else
        std::map<SOCKET_HANDLE, std::shared_ptr<socket_base>> tasks_;
        int epoll_set_;
      std::thread epoll_thread_;
#endif//_WIN32
    };
}

#endif//__VDS_NETWORK_NETWORK_SERVICE_P_H_
