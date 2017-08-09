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

std::string vds::network_service::to_string(const sockaddr & from, socklen_t from_len)
{
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  
  auto result = getnameinfo(&from, from_len,
    hbuf, sizeof hbuf,
    sbuf, sizeof sbuf,
    NI_NUMERICHOST | NI_NUMERICSERV);
  
  return std::string(hbuf) + ":" + std::string(sbuf);
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
: epoll_count_(0)
#endif
{
}


vds::_network_service::~_network_service()
{
}

void vds::_network_service::start(const service_provider & sp)
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
        this->work_threads_.push_back(new std::thread([this, sp] { this->thread_loop(sp); }));
    }

#else
    this->epoll_set_ = epoll_create(100);
    if(0 > this->epoll_set_){
      throw std::runtime_error("Out of memory for epoll_create");
    }
#endif
}

void vds::_network_service::stop(const service_provider & sp)
{
    try {
      sp.get<logger>()->trace(sp, "Stopping network service");
      
#ifndef _WIN32
        this->epoll_future_.wait();
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
    catch (const std::exception & ex) {
      sp.get<logger>()->error(sp, "Failed stop network service %s", ex.what());
    }
    catch (...) {
      sp.get<logger>()->error(sp, "Unhandled error at stopping network service");
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

          if (pOverlapped != NULL) {
            _socket_task::from_overlapped(pOverlapped)->error(errorCode);
            continue;
          }
          else {
            sp.get<logger>()->error(sp, std::system_error(errorCode, std::system_category(), "GetQueuedCompletionStatus").what());
            return;
          }
        }
        try {
          _socket_task::from_overlapped(pOverlapped)->process(dwBytesTransfered);
        }
        catch (const std::exception & ex) {
          auto p = sp.get_property<unhandled_exception_handler>(
            service_provider::property_scope::any_scope);
          if (nullptr != p) {
            p->on_error(sp, std::make_shared<std::exception>(ex));
          }
          else {
            sp.get<logger>()->error(
              sp,
              "IO Task error: %s",
              ex.what());
          }
        }
        catch (...) {
          auto p = sp.get_property<unhandled_exception_handler>(
            service_provider::property_scope::any_scope);
          if (nullptr != p) {
            p->on_error(sp, std::make_shared<std::runtime_error>("Unexcpected error"));
          }
          else {
            sp.get<logger>()->error(
              sp,
              "IO Task error: Unexcpected error");
          }
        }
    }
}
#else

void vds::_network_service::associate(
  const service_provider & sp,
  SOCKET_HANDLE s,
  _socket_task * handler,
  uint32_t event_mask)
{
  std::unique_lock<std::mutex> lock(this->epoll_mutex_);
  
  struct epoll_event event_data;
  event_data.events = event_mask;
  event_data.data.ptr = handler;
  
  int result = epoll_ctl(this->epoll_set_, EPOLL_CTL_ADD, s, &event_data);
  if(0 > result) {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "epoll_ctl");
  }
  
  if(0 == this->epoll_count_++){
    this->epoll_future_ = std::async(std::launch::async,
      [this, sp] {
        while(!sp.get_shutdown_event().is_shuting_down()){
          struct epoll_event events[64];
          
          std::unique_lock<std::mutex> lock(this->epoll_mutex_);
          auto result = epoll_wait(this->epoll_set_, events, sizeof(events) / sizeof(events[0]), 1000);
          if(0 > result){
            auto error = errno;
            if(EINTR == error){
              continue;
            }
            if(0 == this->epoll_count_){
              break;
            }
            throw std::system_error(error, std::system_category(), "epoll_wait");
          }
          else if(0 < result){
            for(int i = 0; i < result; ++i){
              ((_socket_task *)events[i].data.ptr)->process(events[i].events);
            }
          }          
        }
    });
  }
}

void vds::_network_service::set_events(
  const service_provider & sp,
  SOCKET_HANDLE s,
  _socket_task * handler,
  uint32_t event_mask)
{
  struct epoll_event event_data;
  event_data.events = event_mask;
  event_data.data.ptr = handler;
  
  int result = epoll_ctl(this->epoll_set_, EPOLL_CTL_MOD, s, &event_data);
  if(0 > result) {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "epoll_ctl");
  }
}

void vds::_network_service::remove_association(
  const service_provider & sp,
  SOCKET_HANDLE s)
{
  std::unique_lock<std::mutex> lock(this->epoll_mutex_);
  
  struct epoll_event event_data;
  event_data.events = 0;
  
  int result = epoll_ctl(this->epoll_set_, EPOLL_CTL_DEL, s, &event_data);
  if(0 > result) {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "epoll_ctl");
  }

  this->epoll_count_--;
}

#endif//_WIN32

