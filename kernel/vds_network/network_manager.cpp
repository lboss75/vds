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

vds::inetwork_manager::inetwork_manager(network_service * owner)
  : owner_(owner)
{
}

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
        log(ll_trace, "Stopping network service");
        
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
        log(ll_error, "Failed stop network service %s", ex->what());
        delete ex;
    }
    catch (...) {
        log(ll_error, "Unexpected error at stopping network service ");
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
          log(ll_error, error_message->what());
          return;
        }
        try {
          socket_task::from_overlapped(pOverlapped)->process(dwBytesTransfered);
        }
        catch (std::exception * ex) {
          log(ll_error, "IO Task error: %s", ex->what());
          delete ex;
        }
    }
}

#endif//_WIN32

