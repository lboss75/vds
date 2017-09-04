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
    class _socket_task;

    class _network_service : public inetwork_service
    {
    public:
        _network_service();
        ~_network_service();

        // Inherited via iservice
        void start(const service_provider &);
        void stop(const service_provider &);
        
        void remove(_socket_task * socket);

#ifdef _WIN32
        void associate(SOCKET_HANDLE s);

#else
        void associate(
          const service_provider & sp,
          SOCKET_HANDLE s,
          _socket_task * handler,
          uint32_t event_mask);
        void set_events(
          const service_provider & sp,
          SOCKET_HANDLE s,
          uint32_t event_mask);
        void remove_association(
          const service_provider & sp,
          SOCKET_HANDLE s);
#endif
        static std::string to_string(const sockaddr_in & from);
        static std::string get_ip_address_string(const sockaddr_in & from);
        
    private:
        friend class network_socket;
        friend class _udp_socket;
        friend class server_socket;
        friend class _read_socket_task;
        friend class _write_socket_task;
        
        timer update_timer_;
        std::map<SOCKET_HANDLE, _socket_task *> tasks_;
        std::mutex tasks_mutex_;

#ifdef _WIN32
        HANDLE handle_;
        void thread_loop(const service_provider & provider);
        std::list<std::thread *> work_threads_;
#else
      int epoll_set_;
      std::thread epoll_thread_;
#endif//_WIN32
    };
}

#endif//__VDS_NETWORK_NETWORK_SERVICE_P_H_
