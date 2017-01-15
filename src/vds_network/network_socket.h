#ifndef __NETWORK_SOCKET_H_
#define __NETWORK_SOCKET_H_

#ifndef _WIN32
/* Libevent. */
#include <event.h>
#endif

#include "mt_service.h"
#include "network_types.h"

namespace vds {
    class network_service;

    class network_socket
    {
    public:
#ifdef _WIN32
      using SOCKET_HANDLE = SOCKET;
#else
      using SOCKET_HANDLE = int;
#endif
      
      network_socket();
      network_socket(const network_socket &) = delete;
      network_socket(network_socket &&);
      ~network_socket();
      
      network_socket & operator = (
        const network_socket &
      ) = delete;

      SOCKET_HANDLE handle() const
      {
        return this->s_;
      }

    private:
      network_socket(SOCKET_HANDLE s);

      SOCKET_HANDLE s_;
    };
    
    class server_socket
    {
    public:
      server_socket(
        int af = AF_INET,
        int type = SOCK_STREAM);

      void start(
          const service_provider & sp,
          network_service * owner,
          const std::string & address,
          int port,
          std::function<void(const network_socket&)> on_connected,
          const error_handler_t & on_error,
          int backlog = SOMAXCONN);

        void stop();

    private:
        class system_resource : public std::enable_shared_from_this<system_resource>
        {
        public:
            system_resource(int af = AF_INET, int type = SOCK_STREAM);
            ~system_resource();

            void start(
                const service_provider & sp,
                network_service * owner,
                const std::string & address,
                int port,
                std::function<void(const network_socket&)> on_connected,
                const error_handler_t & on_error,
                int backlog = SOMAXCONN);

            void stop();

        private:
#ifdef _WIN32
            SOCKET s_;
            async_result accept_thread_;
#else
	    /* The socket accept event. */
	    struct event ev_accept_;
            int s_;
#endif//_WIN32
        };

        std::shared_ptr<system_resource> handle_;
    };
}

#endif//__NETWORK_SOCKET_H_