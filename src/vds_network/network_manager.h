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
/*
        // connected_handler(const network_socket &)
        template <typename connected_handler>
        void start_server(
          const service_provider & sp,
          const std::string & address,
          uint16_t port,
          connected_handler on_connected,
          const error_handler_t & on_error);

        udp_socket bind(
          const service_provider & sp,
          const std::string & address,
          uint16_t port,
          const std::function<void(const udp_socket &, const sockaddr_in &, const void *, size_t)> & on_incoming_datagram,
          const error_handler_t & on_error,
          size_t max_dgram_size = 4 * 1024);

        network_socket connect(
            const std::string & address,
            uint16_t port
        );
        
  */        

    private:
      friend class socket_server;
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
        
    private:
      /*
#ifndef _WIN32
  template <typename data_handler>
  struct read_data {
    void * buffer_;
    size_t size_;
    data_handler done_;
    error_handler_t & on_error_;
    struct event event_;
    
    static void callback(int fd, short event, void *arg)
    {
      std::unique_ptr<read_data> buf(reinterpret_cast<read_data *>(arg));
      int len = read(fd, buf->buffer_, buf->size_);
      if (len < 0) {
        int error = errno;
        buf->on_error_(
          new vds::system_error(error, std::system_category()));
        return;
      }
      buf->done_(buf->buffer_, len);
    }
};
#endif//_WIN32
*/
        

    private:
        friend class inetwork_manager;
        friend class network_socket;
        friend class udp_socket;
        friend class server_socket;

/*        std::list<server_socket> servers_;

        void write_async(
          const std::function<void(void)> & done,
          const error_handler_t & on_error,
          const service_provider & sp,
          const network_socket & s,
          const void * data,
          size_t size) const;
          
        void read_async(
          const std::function<void(size_t)> & done,
          const error_handler_t & on_error,
          const service_provider & sp,
          const network_socket & s,
          void * buffer,
          size_t size) const;
        
        void start_listen(
          const vds::service_provider & sp,
          const udp_socket & s,
          const sockaddr_in & addr,
          const std::function< void(const udp_socket &, const sockaddr_in &, const void *, size_t ) >& on_incoming_datagram,
          const error_handler_t & on_error,
          size_t max_dgram_size);

        void start_server(
            const service_provider & sp,
            const std::string & address,
            uint16_t port,
            std::function<void(const network_socket &)> on_connected,
            const error_handler_t & on_error);

        udp_socket bind(
            const service_provider & sp,
            const std::string & address,
            uint16_t port,
            const std::function<void(const udp_socket &, const sockaddr_in &, const void *, size_t)> & on_incoming_datagram,
          const error_handler_t & on_error,
            size_t max_dgram_size = 4 * 1024);

        void udp_write_async(
            const std::function<void(size_t)> & done,
            const error_handler_t & on_error,
            const service_provider & sp,
            const udp_socket & s,
            const sockaddr_in & to,
            const void * data,
            size_t size) const;

#ifdef _WIN32
        class io_task
        {
        public:
            io_task();
            virtual ~io_task();

            virtual void process(DWORD dwBytesTransfered) = 0;

            static io_task * from_overlapped(OVERLAPPED * pOverlapped) {
                return reinterpret_cast<io_task *>((uint8_t *)pOverlapped - offsetof(io_task, overlapped_));
            }

        protected:
            OVERLAPPED overlapped_;
            WSABUF wsa_buf_;
        };

        class network_socket_io_task : public io_task
        {
        public:
            network_socket_io_task(const network_socket & s);

        protected:
            network_socket s_;
        };

        class write_task : public network_socket_io_task
        {
        public:
            write_task(
              const network_socket & s,
              const void * data,
              size_t size,
              const std::function<void(void)> & done,
              const error_handler_t & on_error);

            void start();
            void process(DWORD dwBytesTransfered) override;

        private:
            const void * data_;
            size_t size_;
            std::function<void(void)> done_;
            error_handler_t on_error_;
            DWORD transfered_;
        };

        class read_task : public network_socket_io_task
        {
        public:
            read_task(
              const network_socket & s,
              void * data,
              size_t size,
              const std::function<void(size_t)> & done,
              const error_handler_t & on_error);

            void start();
            void process(DWORD dwBytesTransfered) override;

        private:
            void * data_;
            size_t size_;
            const std::function<void(size_t)> done_;
            error_handler_t on_error_;
        };

        class udp_socket_io_task : public io_task
        {
        public:
            udp_socket_io_task(const udp_socket & s);

        protected:
            udp_socket s_;
        };

        class udp_write_task : public udp_socket_io_task
        {
        public:
            udp_write_task(
              const udp_socket & s,
              const sockaddr_in & to,
              const void * data,
              size_t size, 
              const std::function<void(size_t)> & done,
              const error_handler_t & on_error);

            void start();
            void process(DWORD dwBytesTransfered) override;

        private:
            sockaddr_in to_;
            const void * data_;
            size_t size_;
            std::function<void(size_t)> done_;
            error_handler_t on_error_;
        };

        class udp_read_task : public udp_socket_io_task
        {
        public:
            udp_read_task(
              const udp_socket & s,
              const sockaddr_in & addr,
              size_t max_dgram_size,
              const std::function< void(const udp_socket &, const sockaddr_in &, const void *, size_t) > & done,
              const error_handler_t & on_error);

            void start();
            void process(DWORD dwBytesTransfered) override;

        private:
            struct sockaddr_in addr_;
            int addr_len_;
            std::vector<uint8_t> buffer_;
            std::function< void(const udp_socket &, const sockaddr_in &, const void *, size_t) > done_;
            error_handler_t on_error_;
        };

#else
        struct listen_data {
          u_int8_t * buffer_;
          size_t size_;
          std::function<void (const udp_socket &, const sockaddr_in &, const void *, size_t ) > done_;
          service_provider sp_;
          udp_socket s_;
          socklen_t clientlen_;
          struct event event_;
          sockaddr_in clientaddr_;
    
          listen_data(
            const vds::service_provider & sp,
            const vds::udp_socket & s,
            u_int8_t * buffer,
            const sockaddr_in & addr,
            size_t size,
            const std::function<void (const vds::udp_socket &, const sockaddr_in &, const void *, size_t ) > & done
          );
    
          static void callback(int fd, short event, void *arg);
        };
        
        std::list<std::unique_ptr<listen_data>> listen_udp_;
#endif//_WIN32
*/
#ifdef _WIN32
        HANDLE handle_;
        void thread_loop(const service_provider & provider);
        void associate(SOCKET s);
#else
        bool dispatch_started_;
        void start_libevent_dispatch();
#endif//_WIN32
        std::list<std::thread *> work_threads_;
    };
}

#endif//__VDS_NETWORK_NETWORK_MANAGER_H_
