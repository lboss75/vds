#ifndef __VDS_NETWORK_NETWORK_SERVICE_P_H_
#define __VDS_NETWORK_NETWORK_SERVICE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <functional>
#include <vector>
#include <future>

#include "service_provider.h"
#include "network_service.h"

namespace vds {
    class network_service;

    class _network_service : public inetwork_service
    {
    public:
        _network_service();
        ~_network_service();

        // Inherited via iservice
        void start(const service_provider &);
        void stop(const service_provider &);
        
#ifdef _WIN32
        void associate(network_socket::SOCKET_HANDLE s);
#endif
        static std::string to_string(const sockaddr_in & from);
        static std::string get_ip_address_string(const sockaddr_in & from);
        
    private:
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
      event_base * base_;
      void start_libevent_dispatch(const service_provider & sp);
      
    private:
#endif//_WIN32
    };
}

#endif//__VDS_NETWORK_NETWORK_SERVICE_P_H_
