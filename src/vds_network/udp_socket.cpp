#include "stdafx.h"
#include "udp_socket.h"
#include "windows_exception.h"
#include "c_exception.h"

vds::udp_socket::udp_socket(
  const service_provider & sp,
  vds::network_service* owner,
#ifdef _WIN32
  SOCKET s,
#else//_WIN32
  int s,
#endif//_WIN32
  const sockaddr_in & addr,
  const std::function< void(const udp_socket &, const sockaddr_in &, const void *, size_t) >& on_incoming_datagram,
  const error_handler_t & on_error,
  size_t max_dgram_size /*= 4 * 1024*/)
: handle_(new vds::network_socket::system_resource(s)), owner_(owner)
{
#ifdef  _WIN32
    owner->associate(s);
#endif //  _WIN32
    owner->start_listen(sp, *this, addr, on_incoming_datagram, on_error, max_dgram_size);
}


void vds::udp_socket::write_async(
  const std::function<void(size_t)> & done,
  const error_handler_t & on_error,
  const service_provider & sp,
  const sockaddr_in & dest_addr,
  const void * data, size_t size) const
{
    this->owner_->udp_write_async(done, on_error, sp, *this, dest_addr, data, size);
}

vds::udp_socket vds::udp_socket::bind(
  const service_provider & sp,
  network_service * owner,
  const std::string & address,
  int port,
  const std::function<void(const vds::udp_socket &, const sockaddr_in&, const void*, size_t)>& on_incoming_datagram,
  const error_handler_t & on_error,
  size_t max_dgram_size)
{
#ifdef _WIN32
    SOCKET s = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);

    if (INVALID_SOCKET == s) {
        auto error = WSAGetLastError();
        throw new windows_exception("create UDP socket", error);
    }
#else
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (0 > s) {
        auto error = errno;
        throw new c_exception("create UDP socket", error);
    }
    
    /*************************************************************/
    /* Allow socket descriptor to be reuseable                   */
    /*************************************************************/
    int on = 1;
    if (0 > setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
        auto error = errno;
        close(s);
        throw new c_exception("Allow socket descriptor to be reuseable", error);
    }

    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for    */
    /* the incoming connections will also be nonblocking since  */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    if (0 > ioctl(s, FIONBIO, (char *)&on)) {
        auto error = errno;
        close(s);
        throw new c_exception("Set socket to be nonblocking", error);
    }    
#endif

    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(address.c_str());
    if (INADDR_NONE == sin.sin_addr.s_addr) {
#ifdef _WIN32
        closesocket(s);
#else//!_WIN32
        close(s);
#endif//_WIN32
        throw new std::runtime_error("The target ip address entered must be a legal IPv4 address");
    }
    sin.sin_port = htons(port);

#ifdef _WIN32
    if (SOCKET_ERROR == ::bind(s, (sockaddr *)&sin, sizeof(sin))) {
        auto error = WSAGetLastError();
        closesocket(s);
        throw new windows_exception("bind UDP socket", error);
    }
#else
    if (0 > ::bind(s, (sockaddr *)&sin, sizeof(sin))) {
        auto error = errno;
        close(s);
        throw new c_exception("create UDP socket", error);
    }
#endif

    return udp_socket(
      sp,
      owner,
      s,
      sin,
      on_incoming_datagram,
      on_error,
      max_dgram_size);
}

