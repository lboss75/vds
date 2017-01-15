#include "stdafx.h"
#include "network_socket.h"
#include "logger.h"
#include "windows_exception.h"
#include "c_exception.h"
//////////////////////////////////////////////////////////////////////
#ifndef _WIN32
vds::network_socket::network_socket(network_service * owner, int s)
#else
vds::network_socket::network_socket(network_service * owner, SOCKET s)
#endif
    : owner_(owner), handle_(new system_resource(s))
{
#ifdef  _WIN32
    owner->associate(s);
#endif //  _WIN32

}

vds::network_socket::network_socket()
{
}

void vds::network_socket::write_async(
  const std::function<void(void)> & done,
  const error_handler_t & on_error,
  const service_provider & sp,
  const void * data,
  size_t size
  ) const
{
    this->owner_->write_async(done, on_error, sp, *this, data, size);
}


void vds::network_socket::read_async(
  const std::function<void(size_t)> & done,
  const error_handler_t & on_error,
  const service_provider & sp,
  void * buffer,
  size_t size) const
{
    this->owner_->read_async(done, on_error, sp, *this, buffer, size);
}

void vds::server_socket::start(
  const vds::service_provider & sp,
  network_service * owner,
  const std::string & address,
  int port,
  std::function< void(const network_socket&) > on_connected,
  const error_handler_t & on_error,
  int backlog)
{
  this->handle_->start(sp, owner, address, port, on_connected, on_error, backlog);
}

void vds::server_socket::stop()
{
    this->handle_->stop();
}

//////////////////////////////////////////////////////////////////////
#ifdef _WIN32
vds::network_socket::system_resource::system_resource(SOCKET s)
#else
vds::network_socket::system_resource::system_resource(int s)
#endif
    : s_(s)
{
}

vds::network_socket::system_resource::~system_resource()
{
#ifdef _WIN32
    if (INVALID_SOCKET != this->s_) {
        closesocket(this->s_);
    }
#else
    if (0 <= this->s_) {
        shutdown(this->s_, 2);
    }
#endif
}

//////////////////////////////////////////////////////////////////////
vds::server_socket::server_socket(int af, int type)
    : handle_(new system_resource(af, type))
{
}

//////////////////////////////////////////////////////////////////////
vds::server_socket::system_resource::system_resource(int af, int type)
{
#ifdef _WIN32
    this->s_ = WSASocket(af, type, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    if (INVALID_SOCKET == this->s_) {
        auto error = WSAGetLastError();
        throw new windows_exception("create socket", error);
    }
#else
    this->s_ = socket(af, type, 0);
    if (this->s_ < 0) {
        auto error = errno;
        throw new c_exception("create socket", error);
    }
    /*************************************************************/
    /* Allow socket descriptor to be reuseable                   */
    /*************************************************************/
    int on = 1;
    if (0 > setsockopt(this->s_, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
        auto error = errno;
        throw new c_exception("Allow socket descriptor to be reuseable", error);
    }

    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for    */
    /* the incoming connections will also be nonblocking since  */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    if (0 > ioctl(this->s_, FIONBIO, (char *)&on)) {
        auto error = errno;
        throw new c_exception("Set socket to be nonblocking", error);
    }
#endif
}

vds::server_socket::system_resource::~system_resource()
{
#ifdef _WIN32
    if (INVALID_SOCKET != this->s_) {
        closesocket(this->s_);
    }
#else
    if (0 <= this->s_) {
        shutdown(this->s_, 2);

    }
#endif
}

#ifdef _WIN32
class windows_wsa_event
{
public:
    windows_wsa_event()
        : handle_(new system_resource())
    {
    }

    void select(SOCKET s, long lNetworkEvents)
    {
        if (SOCKET_ERROR == WSAEventSelect(s, this->handle(), FD_ACCEPT)) {
            auto error = WSAGetLastError();
            throw new vds::windows_exception("WSAEventSelect", error);
        }
    }

    WSAEVENT handle() const {
        return this->handle_->handle();
    }

private:
    class system_resource : public std::enable_shared_from_this<system_resource>
    {
    public:
        system_resource()
        {
            this->handle_ = WSACreateEvent();
            if (WSA_INVALID_EVENT == this->handle_) {
                auto error = WSAGetLastError();
                throw new vds::windows_exception("WSACreateEvent", error);
            }
        }

        ~system_resource()
        {
            if (WSA_INVALID_EVENT != this->handle_) {
                WSACloseEvent(this->handle_);
            }
        }

        WSAEVENT handle() const {
            return this->handle_;
        }

    private:
        WSAEVENT handle_;
    };

    std::shared_ptr<system_resource> handle_;
};

#else
struct accept_data
{
  vds::network_service * owner_;
  std::function< void(const vds::network_socket&) > on_connected_;
  event * ev_accept_;
};

#endif//_WIN32
void vds::server_socket::system_resource::start(
  const vds::service_provider & sp,
  network_service * owner,
  const std::string& address,
  int port,
  std::function< void(const vds::network_socket&) > on_connected,
  const error_handler_t & on_error,
  int backlog)
{
    //bind to address
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

#ifdef _WIN32
    if (SOCKET_ERROR == ::bind(this->s_, (struct sockaddr *)&addr, sizeof(addr))) {
        auto error = WSAGetLastError();
        throw new windows_exception("bind", error);
    }

    if (SOCKET_ERROR == ::listen(this->s_, SOMAXCONN)) {
        auto error = WSAGetLastError();
        throw new windows_exception("listen socket", error);
    }

    this->accept_thread_.begin(sp, [this, sp, owner, on_connected] {
        logger log(sp, "vds::server_socket::listen");

        windows_wsa_event accept_event;
        accept_event.select(this->s_, FD_ACCEPT);

        WSANETWORKEVENTS WSAEvents;
        HANDLE events[2];
        events[0] = sp.get_shutdown_event().windows_handle();
        events[1] = accept_event.handle();

        //Accept thread will be around to look for accept event, until a Shutdown event is not Signaled.
        for (;;) {
            auto result = WSAWaitForMultipleEvents(2, events, FALSE, INFINITE, FALSE);
            if (WAIT_OBJECT_0 == result) {
                break;
            }

            if ((WAIT_OBJECT_0 + 1) == result) {
                WSAEnumNetworkEvents(this->s_, accept_event.handle(), &WSAEvents);
                if ((WSAEvents.lNetworkEvents & FD_ACCEPT) && (0 == WSAEvents.iErrorCode[FD_ACCEPT_BIT])) {
                    //Process it
                    sockaddr_in client_address;
                    int client_address_length = sizeof(client_address);

                    auto socket = accept(this->s_, (sockaddr*)&client_address, &client_address_length);
                    if (INVALID_SOCKET != socket) {
                        network_socket s(owner, socket);
                        on_connected(s);
                    }
                }
            }
        }
    }, [sp]() {
        logger log(sp, "vds::server_socket::listen");
    },
      on_error);
#else
    if (0 > ::bind(this->s_, (struct sockaddr *)&addr, sizeof(addr))) {
        auto error = errno;
        throw new c_exception("bind", error);
    }

    if (0 > ::listen(this->s_, SOMAXCONN)) {
        auto error = errno;
        throw new c_exception("listen", error);
    }

    /* Set the socket to non-blocking, this is essential in event
    * based programming with libevent. */

    auto flags = fcntl(this->s_, F_GETFL);
    if (0 > flags) {
        auto error = errno;
        throw new c_exception("get server socket flags", error);
    }

    flags |= O_NONBLOCK;
    if (0 > fcntl(this->s_, F_SETFL, flags)) {
        auto error = errno;
        throw new c_exception("set server socket to non-blocking", error);
    }

    auto data = new accept_data { owner, on_connected, &this->ev_accept_ };
    /* We now have a listening socket, we create a read event to
      * be notified when a client connects. */
    event_set(&this->ev_accept_, this->s_, EV_READ | EV_PERSIST,
        [](int fd, short event, void *arg)
    {
        auto data = reinterpret_cast<accept_data *>(arg);

        sockaddr_in client_addr;
        socklen_t   len = 0;

        // Accept incoming connection
        int sock = accept(fd, reinterpret_cast<sockaddr *>(&client_addr), &len);
        if (sock < 1) {
            return;
        }
        
        /* Set the socket to non-blocking, this is essential in event
        * based programming with libevent. */

        auto flags = fcntl(sock, F_GETFL);
        if (0 > flags) {
          auto error = errno;
          throw new c_exception("get socket flags", error);
        }

        flags |= O_NONBLOCK;
        if (0 > fcntl(sock, F_SETFL, flags)) {
          auto error = errno;
          throw new c_exception("set server socket to non-blocking", error);
        }
        
        /*************************************************************/
        /* Set socket to be nonblocking. All of the sockets for    */
        /* the incoming connections will also be nonblocking since  */
        /* they will inherit that state from the listening socket.   */
        /*************************************************************/
        //int on = 1;
        //if (0 > ioctl(sock, FIONBIO, (char *)&on)) {
        //    auto error = errno;
        //    throw new c_exception("Set socket to be nonblocking", error);
        //}
        
        data->on_connected_(network_socket(data->owner_, sock));

        // Reschedule server event
        event_add(data->ev_accept_, NULL);
    }
    , data);
    event_add(&this->ev_accept_, NULL);
    
    owner->start_libevent_dispatch();
#endif
}

void vds::server_socket::system_resource::stop()
{
#ifdef _WIN32
    this->accept_thread_.wait();
#endif
}

