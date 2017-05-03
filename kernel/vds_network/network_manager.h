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

namespace vds {
    class network_service;

    class inetwork_manager {
    public:

    };

    class network_service : public iservice_factory, public inetwork_manager
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
        static std::string to_string(const sockaddr_in & from);
        static std::string get_ip_address_string(const sockaddr_in & from);
        
        void register_server_socket(network_socket::SOCKET_HANDLE s);
        void unregister_server_socket(network_socket::SOCKET_HANDLE s);
    private:
        friend class network_socket;
        friend class udp_socket;
        friend class server_socket;

        std::list<network_socket::SOCKET_HANDLE> server_sockets_;
#ifdef _WIN32
        HANDLE handle_;
        void thread_loop(const service_provider & provider);
        std::list<std::thread *> work_threads_;
#else
        bool dispatch_started_;
    public:
      std::future<void> libevent_future_;
      event_base * base_;
      void start_libevent_dispatch(const service_provider & sp);
      
    private:
#endif//_WIN32
    };
}

#endif//__VDS_NETWORK_NETWORK_MANAGER_H_
