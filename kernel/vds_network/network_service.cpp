/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "network_service.h"
#include "network_service_p.h"
#include "tcp_network_socket.h"
#include "udp_socket.h"
#include "service_provider.h"
#include "logger.h"
#include <iostream>
#include "socket_task_p.h"

vds::network_service::network_service()
: impl_(new _network_service())
{
}

vds::network_service::~network_service()
{
  delete this->impl_;
}

void vds::network_service::register_services(service_registrator & registator)
{
    registator.add_service<inetwork_service>(this->impl_);
}

void vds::network_service::start(const service_provider & sp)
{
  this->impl_->start(sp);
}

void vds::network_service::stop(const service_provider & sp)
{
  this->impl_->stop(sp);
}

std::string vds::network_service::to_string(const sockaddr_in & from)
{
  return get_ip_address_string(from) + ":" + std::to_string(ntohs(from.sin_port));
}

std::string vds::network_service::get_ip_address_string(const sockaddr_in & from)
{
  char buffer[20];
  int len = sizeof(buffer);

  inet_ntop(from.sin_family, &(from.sin_addr), buffer, len);

  return buffer;
}
/////////////////////////////////////////////////////////////////////////////
vds::_network_service::_network_service()
#ifndef _WIN32
: dispatch_started_(false), base_(nullptr)
#endif
{
}


vds::_network_service::~_network_service()
{
}

void vds::_network_service::start(const service_provider & provider)
{
#ifdef _WIN32
    //Initialize Winsock
    WSADATA wsaData;
    if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        auto error = WSAGetLastError();
        throw std::system_error(error, std::system_category(), "Initiates Winsock");
    }

    this->handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    if (NULL == this->handle_) {
        auto error = WSAGetLastError();
        throw std::system_error(error, std::system_category(), "Create I/O completion port");
    }

    //Create worker threads
    for (unsigned int i = 0; i < 2 * std::thread::hardware_concurrency(); ++i) {
        this->work_threads_.push_back(new std::thread([this, provider] { this->thread_loop(provider); }));
    }

#else
    /* Initialize libevent. */
    event_init();
    
    this->base_ = event_base_new();
#endif
}

#ifndef _WIN32
void vds::_network_service::start_libevent_dispatch(const service_provider & sp)
{
  if(!this->dispatch_started_) {
    this->dispatch_started_ = true;
    
    this->libevent_future_ = std::async(std::launch::async,
      [this, sp] {
        timeval ten_sec;
        memset(&ten_sec, 0, sizeof(ten_sec));
        ten_sec.tv_sec = 10;
        while(!sp.get_shutdown_event().is_shuting_down()){
          event_base_loopexit(this->base_, &ten_sec);
          event_base_dispatch(this->base_);
        }
    });
  }
}
#endif

void vds::_network_service::stop(const service_provider & sp)
{
    try {
      sp.get<logger>()->trace(sp, "Stopping network service");
      
#ifndef _WIN32
        do{
          event_base_loopbreak(this->base_);
        }
        while(std::future_status::ready != this->libevent_future_.wait_for(std::chrono::seconds(5)));
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
    catch (...) {
      sp.get<logger>()->error(sp, "Failed stop network service %s", exception_what(std::current_exception()).c_str());
    }
}

#ifdef _WIN32
void vds::_network_service::associate(SOCKET_HANDLE s)
{
  if (NULL == CreateIoCompletionPort((HANDLE)s, this->handle_, NULL, 0)) {
    auto error = GetLastError();
    throw std::system_error(error, std::system_category(), "Associate with input/output completion port");
  }
}

void vds::_network_service::thread_loop(const service_provider & sp)
{
    while (!sp.get_shutdown_event().is_shuting_down()) {
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
          sp.get<logger>()->error(sp, error_message->what());
          return;
        }
        try {
          _socket_task::from_overlapped(pOverlapped)->process(dwBytesTransfered);
        }
        catch (...) {
          auto p = sp.get_property<unhandled_exception_handler>(
            service_provider::property_scope::any_scope);
          if (nullptr != p) {
            p->on_error(sp, std::current_exception());
          }
          else {
            sp.get<logger>()->error(
              sp,
              "IO Task error: %s",
              exception_what(std::current_exception()).c_str());
          }
        }
    }
}

#endif//_WIN32

