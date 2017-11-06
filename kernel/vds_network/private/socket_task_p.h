#ifndef __VDS_NETWORK_SOCKET_TASK_P_H_
#define __VDS_NETWORK_SOCKET_TASK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_types_p.h"
#include "network_service.h"
#include "private/network_service_p.h"

namespace vds {
  class service_provider;
  
  class _socket_task : public std::enable_shared_from_this<_socket_task>
  {
  public:
    _socket_task();
    virtual ~_socket_task();
    
#ifdef _WIN32
    virtual void process(DWORD dwBytesTransfered) = 0;
    virtual void error(DWORD error_code) = 0;

    static _socket_task * from_overlapped(OVERLAPPED * pOverlapped) {
        return reinterpret_cast<_socket_task *>((uint8_t *)pOverlapped - offsetof(_socket_task, overlapped_));
    }
#else
    virtual void process(uint32_t events) = 0;
#endif//_WIN32

  protected:
#ifdef _WIN32
    OVERLAPPED overlapped_;
    WSABUF wsa_buf_;
#else

#endif//_WIN32

  };

  template <typename implementation_class>
  class _socket_task_impl : public _socket_task
  {
    using this_class = _socket_task_impl<implementation_class>;
  public:
    _socket_task_impl(
      const service_provider & sp,
      SOCKET_HANDLE s)
    : sp_(sp), s_(s),
      network_service_(static_cast<_network_service *>(sp.get<inetwork_service>())),
      event_masks_(EPOLLIN | EPOLLET),
      read_status_(read_status_t::bof),
      write_status_(write_status_t::bof)
    {
    }

    ~_socket_task_impl()
    {
      if(EPOLLET != this->event_masks_){
        this->network_service_->remove_association(this->sp_, this->s_);
      }
    }

    void start()
    {
      this->network_service_->associate(
          this->sp_,
          this->s_,
          this->shared_from_this(),
          this->event_masks_);
    }

    void stop()
    {
      if(EPOLLET != this->event_masks_){
        this->event_masks_ = EPOLLET;
        this->network_service_->remove_association(this->sp_, this->s_);
      }
    }

    void process(uint32_t events) override
    {
      if(EPOLLOUT == (EPOLLOUT & events)){
        this->change_mask(0, EPOLLOUT);

        std::lock_guard<std::mutex> lock(this->write_mutex_);

        if(write_status_t::waiting_socket != this->write_status_){
          throw std::runtime_error("Invalid design");
        }

        static_cast<implementation_class *>(this)->write_data();
      }

      if(EPOLLIN == (EPOLLIN & events)){
        this->change_mask(0, EPOLLIN);
        static_cast<implementation_class *>(this)->read_data();
        this->change_mask(EPOLLIN);
      }
    }

  protected:
    SOCKET_HANDLE s_;
    service_provider sp_;
    class _network_service * network_service_;

    enum class read_status_t {
      bof,
      waiting_socket,
      eof
    };

    std::mutex read_mutex_;
    read_status_t read_status_;

    enum class write_status_t {
      bof,
      waiting_socket,
      eof
    };

    std::mutex write_mutex_;
    write_status_t write_status_;

    std::mutex event_masks_mutex_;
    uint32_t event_masks_;

    void change_mask(uint32_t set_events, uint32_t clear_events = 0)
    {
      std::unique_lock<std::mutex> lock(this->event_masks_mutex_);
      auto need_create = (EPOLLET == this->event_masks_);
      this->event_masks_ |= set_events;
      this->event_masks_ &= ~clear_events;

      if(!need_create && EPOLLET != this->event_masks_){
        this->network_service_->set_events(this->sp_, this->s_, this->event_masks_);
      }
      else if (EPOLLET == this->event_masks_){
        this->network_service_->remove_association(this->sp_, this->s_);
      }
      else {
        this->network_service_->associate(this->sp_, this->s_, this->shared_from_this(), this->event_masks_);
      }
    }
  };
}

#endif // __VDS_NETWORK_SOCKET_TASK_P_H_
