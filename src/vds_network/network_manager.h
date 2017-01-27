#ifndef __VDS_NETWORK_NETWORK_MANAGER_H_
#define __VDS_NETWORK_NETWORK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <functional>
#include <vector>

#include "func_utils.h"
#include "service_provider.h"
#include "network_socket.h"
#include "udp_socket.h"

namespace vds {
    class network_service;

    class inetwork_manager {
    public:
        inetwork_manager(network_service * owner);

    private:
      friend class socket_server;
      friend class socket_connect;

      template<
        typename done_method_type,
        typename error_method_type
      >
        friend  class accept_socket_task;

      network_service * owner_;
    };

    class network_service : public iservice
    {
    public:
        network_service();
        ~network_service();

        // Inherited via iservice
        void register_services(service_registrator &) override;
        void start(const service_provider &) override;
        void stop(const service_provider &) override;
#ifdef _WIN32
        void associate(network_socket::SOCKET_HANDLE s);
#endif
    private:
        friend class inetwork_manager;
        friend class network_socket;
        friend class udp_socket;
        friend class server_socket;

#ifdef _WIN32
        HANDLE handle_;
        void thread_loop(const service_provider & provider);
        std::list<std::thread *> work_threads_;
#else
        bool dispatch_started_;
    public:
      std::future<void> libevent_future_;
      void start_libevent_dispatch();
#endif//_WIN32
    };
}

#endif//__VDS_NETWORK_NETWORK_MANAGER_H_
