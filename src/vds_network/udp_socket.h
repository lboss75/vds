#ifndef __UDP_SOCKET_H_
#define __UDP_SOCKET_H_

#include <memory>

namespace vds {
/*
    class udp_socket
    {
    public:
        void write_async(
          const std::function<void(size_t)> & done,
          const error_handler_t & on_error,
          const service_provider & sp,
          const sockaddr_in & dest_addr,
          const void * data, size_t size) const;

        static vds::udp_socket bind(
          const service_provider & sp,
          network_service * owner,
          const std::string & address,
          int port,
          const std::function<void(const udp_socket & , const sockaddr_in &, const void *, size_t )> & on_incoming_datagram,
          const error_handler_t & on_error,
          size_t max_dgram_size = 4 * 1024);

#ifdef _WIN32
        SOCKET
#else//_WIN32
        int 
#endif//_WIN32
          handle() const
          {
            return this->handle_->s_;
          }
    private:
      udp_socket(
        const service_provider & sp,
        network_service * owner,
#ifdef _WIN32
        SOCKET s,
#else//_WIN32
        int s,
#endif//_WIN32
        const sockaddr_in & addr,
        const std::function<void(const udp_socket &, const sockaddr_in &, const void *, size_t)> & on_incoming_datagram,
        const error_handler_t & on_error,
        size_t max_dgram_size = 4 * 1024
      );
      
      network_service * owner_;
      std::shared_ptr<network_socket::system_resource> handle_;

    };
    */
}

#endif//__UDP_SOCKET_H_