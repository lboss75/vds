/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "network_manager.h"
#include "network_socket.h"
#include "udp_socket.h"
#include "service_provider.h"
#include "logger.h"
#include <iostream>

vds::network_service::network_service()
#ifndef _WIN32
: dispatch_started_(false)
#endif
{
}


vds::network_service::~network_service()
{
}

void vds::network_service::register_services(service_registrator & registator)
{
    registator.add_factory<inetwork_manager>([this] (bool & is_scopped) {
        is_scopped = true;
        return inetwork_manager(this);
    });
}

void vds::network_service::start(const service_provider & provider)
{
#ifdef _WIN32
    //Initialize Winsock
    WSADATA wsaData;
    if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        auto error = WSAGetLastError();
        throw new std::system_error(error, std::system_category(), "Initiates Winsock");
    }

    this->handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    if (NULL == this->handle_) {
        auto error = WSAGetLastError();
        throw new std::system_error(error, std::system_category(), "Create I/O completion port");
    }

    //Create worker threads
    for (unsigned int i = 0; i < 2 * std::thread::hardware_concurrency(); ++i) {
        this->work_threads_.push_back(new std::thread([this, provider] { this->thread_loop(provider); }));
    }

#else
    /* Initialize libevent. */
    event_init();
    
    //this->start_libevent_dispatch();
#endif
}

#ifndef _WIN32
void vds::network_service::start_libevent_dispatch()
{
  if(!this->dispatch_started_) {
    this->dispatch_started_ = true;
    
    this->libevent_future_ = std::async(std::launch::async,
      [] {
        /* Start the libevent event loop. */
        event_dispatch();
    });
  }
}
#endif

void vds::network_service::stop(const service_provider & sp)
{
    logger log(sp, "vds::network_service::stop");

    try {
        log(trace("Stopping network service"));
        
#ifndef _WIN32
        event_loopbreak();
        this->libevent_future_.wait_for(
          std::chrono::seconds(5));
#else
        for (auto p : this->work_threads_) {
            p->join();
            delete p;
        }
#endif
        
#ifdef _WIN32

        if (NULL != this->handle_) {
            CloseHandle(this->handle_);
        }

        WSACleanup();
#endif
    }
    catch (const std::exception * ex) {
        log(error("Failed stop network service ") << *ex);
        delete ex;
    }
    catch (...) {
        log(error("Unexpected error at stopping network service "));
    }
}

#ifdef _WIN32
void vds::network_service::associate(network_socket::SOCKET_HANDLE s)
{
  if (NULL == CreateIoCompletionPort((HANDLE)s, this->handle_, NULL, 0)) {
    auto error = GetLastError();
    throw new std::system_error(error, std::system_category(), "Associate with input/output completion port");
  }
}
#endif//_WIN32
/*
#ifndef _WIN32
struct write_data {
    int s_;
    const void * buffer_;
    size_t offset_;
    size_t size_;
    std::function<void(void)> done_;
    vds::service_provider sp_;
    struct event event_;
    
    // Handle client responce {{{
    static void callback(int fd, short event, void *arg)
    {
      write_data * buf = reinterpret_cast<write_data *>(arg);
      int len = write(fd, reinterpret_cast<const uint8_t *>(buf->buffer_) + buf->offset_, buf->size_ - buf->offset_);
      if (len < 0) {
        int error = errno;
        buf->sp_.handle_error(new vds::c_exception("write socket", error));
        delete buf;
        return;
      }
      
      buf->offset_ += len;
      if (buf->offset_ < buf->size_) {
        event_set(&buf->event_, buf->s_, EV_WRITE, &write_data::callback, buf);
        event_add(&buf->event_, NULL);
        
      }
      else {
        buf->done_();
        delete buf;
      }
    }
};
#endif//_WIN32

void vds::network_service::write_async(
  const std::function<void(void)> & done,
  const error_handler_t & on_error,
  const service_provider & sp,
  const network_socket & s,
  const void * data,
  size_t size
  ) const
{
#ifdef _WIN32
    auto task = new write_task(s, data, size, done, on_error);
    task->start();
#else
    auto buf = new write_data{ s.handle(), data, 0, size, 
      [this, done, sp, on_error](){
        async_result result;
        result.begin(sp, done, [](){}, on_error);
      },
      sp
    };

    // Schedule client event
    event_set(&buf->event_, s.handle(), EV_WRITE, &write_data::callback, buf);
    event_add(&buf->event_, NULL);
#endif//_WIN32
}

#ifndef _WIN32
struct read_data {
    void * buffer_;
    size_t size_;
    std::function<void(size_t)> done_;
    vds::service_provider sp_;
    struct event event_;
    
    static void callback(int fd, short event, void *arg)
    {
      std::unique_ptr<read_data> buf(reinterpret_cast<read_data *>(arg));
      int len = read(fd, buf->buffer_, buf->size_);
      if (len < 0) {
        int error = errno;
        buf->sp_.handle_error(new vds::c_exception("read socket", error));
        return;
      }
      buf->done_(len);
    }
};
#endif//_WIN32


void vds::network_service::read_async(
  const std::function<void(size_t)> & done,
  const error_handler_t & on_error,
  const service_provider & sp,
  const network_socket & s,
  void * buffer,
  size_t size
  ) const
{
#ifdef _WIN32
    auto task = new read_task(s, buffer, size, done, on_error);
    task->start();
#else  
    auto buf = new read_data{ buffer, size, 
      [this, done, sp, on_error](size_t size){
        async_result result;
        result.begin(
          sp,
          [done, size]() { done(size); },
          []() { },
          on_error
        );
      },
      sp
    };
        
    event_set(&buf->event_, s.handle(), EV_READ, &read_data::callback, buf);
    // Schedule client event
    event_add(&buf->event_, NULL);
#endif//_WIN32
}

void vds::network_service::start_server(
  const service_provider & sp,
  const std::string & address,
  uint16_t port,
  std::function<void(const network_socket&)> on_connected,
  const error_handler_t & on_error
    )
{
    server_socket listen_socket(AF_INET, SOCK_STREAM);
    servers_.push_back(listen_socket);

    listen_socket.start(
      sp,
      this,
      address,
      port,
      on_connected,
      on_error);
}

vds::udp_socket vds::network_service::bind(
  const service_provider & sp,
  const std::string & address,
  uint16_t port,
  const std::function<void(const vds::udp_socket &, const sockaddr_in &, const void *, size_t)> & on_incoming_datagram,
  const error_handler_t & on_error,
  size_t max_dgram_size
)
{
    return udp_socket::bind(sp, this, address, port, on_incoming_datagram, on_error, max_dgram_size);
}

#ifndef _WIN32
struct udp_write_data {
    int s_;
    const void * buffer_;
    size_t size_;
    std::function<void(size_t)> done_;
    vds::service_provider sp_;
    struct event event_;
    sockaddr_in to_;
    
    udp_write_data(
      const vds::service_provider & sp,
      int s,
      const sockaddr_in & to,
      const void * data,
      size_t size,
      const std::function<void(size_t)>& done
    ) : sp_(sp), done_(done), s_(s), to_(to),
    buffer_(data), size_(size)    
    {
    }
    
    // Handle client responce {{{
    static void callback(int fd, short event, void *arg)
    {
      auto buf = reinterpret_cast<udp_write_data *>(arg);
      int len = sendto(
        fd,
        reinterpret_cast<const uint8_t *>(buf->buffer_),
        buf->size_,
        0,
        (sockaddr *)&buf->to_,
        sizeof(buf->to_)
      );
      if (len < 0) {
        int error = errno;
        buf->sp_.handle_error(new vds::c_exception("write socket", error));
        delete buf;
        return;
      }
      
      buf->done_(len);
      delete buf;
    }
};
#endif//_WIN32

void vds::network_service::udp_write_async(
  const std::function<void(size_t)>& done,
  const error_handler_t & on_error,
  const service_provider & sp,
  const udp_socket & s,
  const sockaddr_in & to,
  const void * data,
  size_t size) const
{
#ifdef _WIN32
    auto task = new udp_write_task(s, to, data, size, done, on_error);
    task->start();
#else//!_WIN32
    auto buf = new udp_write_data(
      sp,
      s.handle(),
      to,
      data, size, 
      [this, done, sp, on_error](size_t size){
        async_result result;
        result.begin(
          sp,
          [done, size]() { done(size); },
          [](){},
          on_error
        );
      });

    // Schedule client event
    event_set(&buf->event_, s.handle(), EV_WRITE, &udp_write_data::callback, buf);
    event_add(&buf->event_, NULL);
#endif//_WIN32
}

#ifndef _WIN32
vds::network_service::listen_data::listen_data(
  const vds::service_provider & sp,
  const vds::udp_socket & s,
  u_int8_t * buffer,
  const sockaddr_in & addr,
  size_t size,
  const std::function<void (const vds::udp_socket &, const sockaddr_in &, const void *, size_t ) > & done
): buffer_(buffer), size_(size), done_(done), sp_(sp),
clientaddr_(addr), clientlen_(sizeof(clientaddr_)),
s_(s)    
{
}
    
void vds::network_service::listen_data::callback(int fd, short event, void *arg)
{
  auto buf(reinterpret_cast<listen_data *>(arg));
  int len = recvfrom(fd, buf->buffer_, buf->size_, 0,
    (struct sockaddr *)&buf->clientaddr_, &buf->clientlen_
  );
  if (len < 0) {
    int error = errno;
    buf->sp_.handle_error(new vds::c_exception("read socket", error));
    return;
  }
  
  buf->done_(buf->s_, buf->clientaddr_, buf->buffer_, len);
  if(!buf->sp_.get_shutdown_event().is_shuting_down()){
    event_set(&buf->event_, buf->s_.handle(), EV_READ, &listen_data::callback, buf);
    event_add(&buf->event_, NULL);
  }
}
#endif//_WIN32

void vds::network_service::start_listen(
  const vds::service_provider & sp,
  const vds::udp_socket& s,
  const sockaddr_in & addr,
  const std::function<void (const udp_socket &, const sockaddr_in &, const void *, size_t ) > & on_incoming_datagram,
  const error_handler_t & on_error,
  size_t max_dgram_size)
{
#ifdef _WIN32
    auto task = new udp_read_task(s, addr, max_dgram_size, on_incoming_datagram, on_error);
    task->start();
#else//!_WIN32
  auto buf = new listen_data(
    sp,
    s,
    new u_int8_t[max_dgram_size],
    addr,
    max_dgram_size,
    on_incoming_datagram);
  
  event_set(&buf->event_, s.handle(), EV_READ, &listen_data::callback, buf);
  event_add(&buf->event_, NULL);
  
  this->start_libevent_dispatch();
 
#endif//_WIN32
}
*/

vds::inetwork_manager::inetwork_manager(network_service * owner)
    : owner_(owner)
{
}
/*
void vds::inetwork_manager::start_server(
  const service_provider & sp,
  const std::string & address,
  uint16_t port,
  std::function<void(const network_socket&)> on_connected,
  const error_handler_t & on_error)
{
    this->owner_->start_server(sp, address, port, on_connected, on_error);
}

vds::udp_socket vds::inetwork_manager::bind(
  const service_provider & sp,
  const std::string & address,
  uint16_t port,
  const std::function<void(const udp_socket &, const sockaddr_in&, const void*, size_t)>& on_incoming_datagram,
  const error_handler_t & on_error,
  size_t max_dgram_size)
{
    return this->owner_->bind(sp, address, port, on_incoming_datagram, on_error, max_dgram_size);
}

vds::network_socket vds::inetwork_manager::connect(const std::string & address, uint16_t port)
{
    // Create socket 
    auto s = socket(PF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (INVALID_SOCKET == s) {
        auto error = WSAGetLastError();
        throw new std::system_error(error, std::system_category(), "create socket");
    }
#else
    if (s < 0) {
        auto error = errno;
        throw new c_exception("create socket", error);
    }
#endif
    // Connexion setting for local connexion 
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    addr.sin_port = htons(port);

#ifdef _WIN32
    // Connect 
    if (SOCKET_ERROR == ::connect(s, (struct sockaddr *)&addr, sizeof(addr))) {
        // As we are in non-blocking mode we'll always have the error 
        // WSAEWOULDBLOCK whichis actually not one 
        auto error = WSAGetLastError();
        if (WSAEWOULDBLOCK != error) {
            closesocket(s);
            throw new std::system_error(error, std::system_category(), "connect");
        }
    }
#else
        // Connect 
        if (0 > ::connect(s, (struct sockaddr *)&addr, sizeof(addr))) {
            auto error = errno;
            throw new c_exception("connect socket", error);
        }
#endif
    return network_socket(this->owner_, s);
}

#ifdef _WIN32
vds::network_service::io_task::io_task()
{
    ZeroMemory(&this->overlapped_, sizeof(this->overlapped_));
}

vds::network_service::io_task::~io_task()
{
}
///////////////////////////////////
vds::network_service::network_socket_io_task::network_socket_io_task(const network_socket & s)
    : s_(s)
{
}


////////////////////////////
vds::network_service::write_task::write_task(const vds::network_socket & s, const void * data, size_t size, const std::function<void(void)> & done, const error_handler_t & on_error)
    : network_socket_io_task(s), data_(data), size_(size), done_(done), transfered_(0), on_error_(on_error)
{
}

void vds::network_service::write_task::start()
{
    this->wsa_buf_.len = this->size_ - this->transfered_;
    this->wsa_buf_.buf = (CHAR *)this->data_ + this->transfered_;

    if (NOERROR != WSASend(this->s_.handle(), &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
            delete this;
            throw new std::system_error(errorCode, std::system_category(), "WSASend failed");
        }
    }
}

void vds::network_service::write_task::process(DWORD dwBytesTransfered)
{
    this->transfered_ += dwBytesTransfered;

    if (this->transfered_ < this->size_) {
        this->start();
    }
    else {
        this->done_();
        delete this;
    }
}
//////////////////////////
vds::network_service::udp_socket_io_task::udp_socket_io_task(const udp_socket & s)
    : s_(s)
{
}
//////////////////////////
vds::network_service::udp_write_task::udp_write_task(
  const udp_socket & s,
  const sockaddr_in & to,
  const void * data,
  size_t size,
  const std::function<void(size_t)> & done,
  const error_handler_t & on_error)
: udp_socket_io_task(s), to_(to), data_(data), size_(size), done_(done)
{
}

void vds::network_service::udp_write_task::start()
{
    this->wsa_buf_.len = this->size_;
    this->wsa_buf_.buf = (CHAR *)this->data_;

    if (NOERROR != WSASendTo(this->s_.handle(), &this->wsa_buf_, 1, NULL, 0, (const sockaddr *)&this->to_, sizeof(this->to_), &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
            delete this;
            throw new std::system_error(errorCode, std::system_category(), "WSASend failed");
        }
    }
}

void vds::network_service::udp_write_task::process(DWORD dwBytesTransfered)
{
    this->done_((size_t)dwBytesTransfered);
    delete this;
}
//////////////////////////
vds::network_service::udp_read_task::udp_read_task(
  const udp_socket & s,
  const sockaddr_in & addr,
  size_t max_dgram_size,
  const std::function< void(const udp_socket &, const sockaddr_in &, const void *, size_t) > & done,
  const error_handler_t & on_error)
    : udp_socket_io_task(s), buffer_(max_dgram_size), done_(done), addr_(addr), addr_len_(sizeof(addr_))
{
}

void vds::network_service::udp_read_task::start()
{
    this->wsa_buf_.len = this->buffer_.size();
    this->wsa_buf_.buf = (CHAR *)this->buffer_.data();

    DWORD flags = 0;
    DWORD numberOfBytesRecvd;
    if (NOERROR != WSARecvFrom(
        this->s_.handle(),
        &this->wsa_buf_,
        1,
        &numberOfBytesRecvd,
        &flags,
        (struct sockaddr*)&this->addr_,
        &this->addr_len_,
        &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
            delete this;
            throw new std::system_error(errorCode, std::system_category(), "WSARecvFrom failed");
        }
    }
}

void vds::network_service::udp_read_task::process(DWORD dwBytesTransfered)
{
    this->done_(this->s_, this->addr_, this->buffer_.data(), dwBytesTransfered);
    delete this;
}

//////////////////////////
vds::network_service::read_task::read_task(
  const vds::network_socket & s,
  void * data,
  size_t size,
  const std::function<void(size_t)> & done,
  const error_handler_t & on_error)
    : network_socket_io_task(s), data_(data), size_(size), done_(done), on_error_(on_error)
{
}

void vds::network_service::read_task::start()
{
    this->wsa_buf_.len = this->size_;
    this->wsa_buf_.buf = (CHAR *)this->data_;

    DWORD flags = 0;
    DWORD numberOfBytesRecvd;
    if(NOERROR != WSARecv(this->s_.handle(), &this->wsa_buf_, 1, &numberOfBytesRecvd, &flags, &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
            delete this;
            throw new std::system_error(errorCode, std::system_category(), "WSARecv failed");
        }
    }
}

void vds::network_service::read_task::process(DWORD dwBytesTransfered)
{
    this->done_(dwBytesTransfered);
    delete this;
}
*/
//////////////////////////
#ifdef _WIN32
void vds::network_service::thread_loop(const service_provider & provider)
{
    logger log(provider, "vds::network_service::thread_loop");

    while (!provider.get_shutdown_event().is_shuting_down()) {
        DWORD dwBytesTransfered = 0;
        void * lpContext = NULL;
        OVERLAPPED * pOverlapped = NULL;

        if (!GetQueuedCompletionStatus(
          this->handle_,
          &dwBytesTransfered,
          (PULONG_PTR)&lpContext,
          &pOverlapped,
          1000)) {
          auto errorCode = GetLastError();
          if (errorCode == WAIT_TIMEOUT) {
            continue;
          }

          std::unique_ptr<std::system_error> error_message(new std::system_error(errorCode, std::system_category(), "GetQueuedCompletionStatus"));
          log(error("") << *error_message.get());
          return;
        }

        try {
          socket_task::from_overlapped(pOverlapped)->process(dwBytesTransfered);
        }
        catch (std::exception * ex) {
          log(error("IO Task error:") << *ex->what());
        }
    }
}

#endif//_WIN32

