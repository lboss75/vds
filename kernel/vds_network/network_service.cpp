/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "network_service.h"
#include "private/network_service_p.h"
#include "tcp_network_socket.h"
#include "udp_socket.h"
#include "service_provider.h"
#include "logger.h"
#include <iostream>
#include "private/socket_task_p.h"

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
    registator.add_service<network_service>(this);
}

void vds::network_service::start(const service_provider & sp)
{
  this->impl_->start(sp);
}

void vds::network_service::stop(const service_provider & sp)
{
  this->impl_->stop(sp);
}

std::future<void> vds::network_service::prepare_to_stop(const service_provider &sp)
{
  return this->impl_->prepare_to_stop(sp);
}


std::string vds::network_service::to_string(const sockaddr & from, size_t from_len)
{
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  
  auto result = getnameinfo(&from, (socklen_t)from_len,
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
#define NETWORK_EXIT 0xA1F8

vds::_network_service::_network_service()
#ifdef _WIN32
  : handle_(NULL)
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
  this->epoll_thread_ = std::thread(
    [this, sp] {
      auto last_timeout_update = std::chrono::steady_clock::now();
      for(;;){
        std::unique_lock<std::mutex> lock(this->tasks_mutex_);
        if(this->tasks_.empty()){
          if(sp.get_shutdown_event().is_shuting_down()){
            break;
          }
          
          this->tasks_cond_.wait(lock);
          continue;
        }
        lock.unlock();
        
        struct epoll_event events[64];
        
        auto result = epoll_wait(this->epoll_set_, events, sizeof(events) / sizeof(events[0]), 1000);
        if(0 > result){
          auto error = errno;
          if(EINTR == error){
            continue;
          }
          
          throw std::system_error(error, std::system_category(), "epoll_wait");
        }
        else if(0 < result){
          for(int i = 0; i < result; ++i){
            lock.lock();
            auto p = this->tasks_.find(events[i].data.fd);
            if(this->tasks_.end() != p){
              std::shared_ptr<_socket_task> handler = p->second;
              lock.unlock();
              
              handler->process(events[i].events);
            }
            else {
              lock.unlock();
            }
          }
        }          
      }
  });
#endif
 
}

void vds::_network_service::stop(const service_provider & sp)
{
    try {
      sp.get<logger>()->trace("network", sp, "Stopping network service");
      
#ifndef _WIN32
      this->tasks_cond_.notify_one();
      if(this->epoll_thread_.joinable()){
        this->epoll_thread_.join();
      }
#else
      for (auto p : this->work_threads_) {
        PostQueuedCompletionStatus(this->handle_, 0, NETWORK_EXIT, NULL);
      }
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
      sp.get<logger>()->error("network", sp, "Failed stop network service %s", ex.what());
    }
    catch (...) {
      sp.get<logger>()->error("network", sp, "Unhandled error at stopping network service");
    }
}

std::future<void> vds::_network_service::prepare_to_stop(const service_provider &sp)
{
  co_return;
  /*
  std::set<SOCKET_HANDLE> processed;
  
  std::unique_lock<std::mutex> lock(this->tasks_mutex_);

  for(;;){
    bool bcontinue = false;
    
    for(auto & p : this->tasks_) {
      if(processed.end() != processed.find(p.first)){
        continue;
      }
      processed.emplace(p.first);
      std::shared_ptr<_socket_task> handler = p.second;
      lock.unlock();
      
      //handler->prepare_to_stop(sp);
      bcontinue = true;
      break;
    }
    
    if(!bcontinue){
      break;
    }
    else {
      lock.lock();
    }
  }
   */
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
    ULONG_PTR lpContext;
    OVERLAPPED * pOverlapped = NULL;

    if (!GetQueuedCompletionStatus(
      this->handle_,
      &dwBytesTransfered,
      &lpContext,
      &pOverlapped,
      INFINITE)) {
      auto errorCode = GetLastError();
      if (errorCode == WAIT_TIMEOUT) {
        continue;
      }

      if (pOverlapped != NULL) {
        sp.get<logger>()->error("network", sp, "GetQueuedCompletionStatus %d error %s", errorCode, std::system_error(errorCode, std::system_category(), "GetQueuedCompletionStatus").what());
        _socket_task::from_overlapped(pOverlapped)->error(errorCode);
        continue;
      }
      else {
        sp.get<logger>()->error("network", sp, "GetQueuedCompletionStatus %d error %s", errorCode, std::system_error(errorCode, std::system_category(), "GetQueuedCompletionStatus").what());
        return;
      }
    }

    if (lpContext == NETWORK_EXIT) {
      return;
    }

    _socket_task::from_overlapped(pOverlapped)->process(dwBytesTransfered);
  }
}
#else

void vds::_network_service::associate(
  SOCKET_HANDLE s,
  const std::shared_ptr<_socket_task> & handler,
  uint32_t event_mask)
{
  struct epoll_event event_data;
  memset(&event_data, 0, sizeof(event_data));
  event_data.events = event_mask;
  event_data.data.fd = s;
  
  int result = epoll_ctl(this->epoll_set_, EPOLL_CTL_ADD, s, &event_data);
  if(0 > result) {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "epoll_ctl(EPOLL_CTL_ADD)");
  }
  
  std::unique_lock<std::mutex> lock(this->tasks_mutex_);
  if(this->tasks_.empty()){
    this->tasks_cond_.notify_one();
  }
  
  this->tasks_[s] = handler;
}

void vds::_network_service::set_events(
  SOCKET_HANDLE s,
  uint32_t event_mask)
{
  struct epoll_event event_data;
  memset(&event_data, 0, sizeof(event_data));
  event_data.events = event_mask;
  event_data.data.fd = s;
  
  int result = epoll_ctl(this->epoll_set_, EPOLL_CTL_MOD, s, &event_data);
  if(0 > result) {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "epoll_ctl(EPOLL_CTL_MOD)");
  }
}

void vds::_network_service::remove_association(
  SOCKET_HANDLE s)
{
  struct epoll_event event_data;
  memset(&event_data, 0, sizeof(event_data));
  event_data.events = 0;
  
  int result = epoll_ctl(this->epoll_set_, EPOLL_CTL_DEL, s, &event_data);
  if(0 > result) {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "epoll_ctl(EPOLL_CTL_DEL)");
  }
  
  std::unique_lock<std::mutex> lock(this->tasks_mutex_);
  this->tasks_.erase(s);
}

#endif//_WIN32

