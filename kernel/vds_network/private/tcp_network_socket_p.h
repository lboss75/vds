#ifndef __VDS_NETWORK_NETWORK_SOCKET_P_H_
#define __VDS_NETWORK_NETWORK_SOCKET_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_types_p.h"
#include "tcp_network_socket.h"
#include "socket_task_p.h"
#include "network_service_p.h"

namespace vds {
  class _network_service;

  class _tcp_network_socket : public std::enable_shared_from_this<_tcp_network_socket>
  {
  public:
    _tcp_network_socket()
    : s_(INVALID_SOCKET),
      incoming_(new continuous_stream<uint8_t>()),
      outgoing_(new continuous_stream<uint8_t>())
    {
    }

    _tcp_network_socket(SOCKET_HANDLE s)
      : s_(s),
      incoming_(new continuous_stream<uint8_t>()),
      outgoing_(new continuous_stream<uint8_t>())
    {
#ifdef _WIN32
      if (INVALID_SOCKET == s) {
#else
      if (s < 0) {
#endif
        throw std::runtime_error("Invalid socket handle");
      }
    }

    _tcp_network_socket(const _tcp_network_socket &) = delete;

    ~_tcp_network_socket()
    {
      this->close();
    }

    void close()
    {
#ifdef _WIN32
      if (INVALID_SOCKET != this->s_) {
        shutdown(this->s_, SD_BOTH);
        closesocket(this->s_);
        this->s_ = INVALID_SOCKET;
      }
#else
      if (0 <= this->s_) {
        shutdown(this->s_, 2);
        this->s_ = -1;
      }
#endif
    }
      
    SOCKET_HANDLE handle() const {
#ifdef _WIN32
      if (INVALID_SOCKET == this->s_) {
#else
      if (0 >= this->s_) {
#endif
        throw std::logic_error("network_socket::handle without open socket");
      }
      return this->s_;
    }
    
    static tcp_network_socket from_handle(SOCKET_HANDLE handle)
    {
      return tcp_network_socket(std::make_shared<_tcp_network_socket>(handle));
    }

    std::shared_ptr<continuous_stream<uint8_t>> incoming() const { return this->incoming_; }
    std::shared_ptr<continuous_stream<uint8_t>> outgoing() const { return this->outgoing_; }

    void start(const service_provider & sp)
    {
      sp.get<logger>()->trace("TCP", sp, "socket start");
      
#ifdef _WIN32
      std::make_shared<_read_socket_task>(sp, this->shared_from_this())->read_async();
      std::make_shared<_write_socket_task>(sp, this->shared_from_this())->write_async();
#else
      std::make_shared<_socket_handler>(sp, this->shared_from_this())->start();
#endif//_WIN32
    }
    
#ifndef _WIN32
    void make_socket_non_blocking()
    {
      auto flags = fcntl (this->handle(), F_GETFL, 0);
      if (flags == -1) {
        throw std::runtime_error("fcntl");
      }

      flags |= O_NONBLOCK;
      auto s = fcntl (this->handle(), F_SETFL, flags);
      if (s == -1) {
        throw std::runtime_error("fcntl");
      }
    }
    void set_timeouts()
    {
//       struct timeval tv;
//       tv.tv_sec = 30;        // 30 Secs Timeout
//       tv.tv_usec = 0;
//       setsockopt(this->handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
    }
#endif//_WIN32

  private:
    SOCKET_HANDLE s_;
    std::shared_ptr<continuous_stream<uint8_t>> incoming_;
    std::shared_ptr<continuous_stream<uint8_t>> outgoing_;

#ifdef _WIN32
    class _read_socket_task : public _socket_task
    {
    public:
      constexpr static size_t BUFFER_SIZE = 1024;

      _read_socket_task(
        const service_provider & sp,
        const std::shared_ptr<_tcp_network_socket> & owner)
        : owner_(owner),
        sp_(sp)
      {
      }

      ~_read_socket_task()
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv closed");
      }

      void read_async()
      {
        this->wsa_buf_.len = BUFFER_SIZE;
        this->wsa_buf_.buf = (CHAR *)this->buffer_;

        this->pthis_ = this->shared_from_this();

        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv");
        DWORD flags = 0;
        DWORD numberOfBytesRecvd;
        if (NOERROR != WSARecv(this->owner_->s_, &this->wsa_buf_, 1, &numberOfBytesRecvd, &flags, &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv error");
            auto pthis = this->pthis_;
            this->pthis_.reset();

            if (WSAESHUTDOWN == errorCode) {
              this->owner_->incoming_->write_async(this->sp_, nullptr, 0).wait(
                [pthis](const service_provider & sp, size_t) {
                },
                [](const service_provider & sp, const std::shared_ptr<std::exception> & error) {
                  sp.unhandled_exception(error);
                },
                this->sp_);
            }
            else {
              this->sp_.unhandled_exception(std::make_unique<std::system_error>(errorCode, std::system_category(), "read from tcp socket"));
            }
            return;
          }
        }
      }

      void check_timeout(const service_provider & sp) override
      {
      }

      void prepare_to_stop(const service_provider & sp) override
      {
      }

    private:
      service_provider sp_;
      std::shared_ptr<_tcp_network_socket> owner_;
      std::shared_ptr<_socket_task> pthis_;
      uint8_t buffer_[BUFFER_SIZE];

      void process(DWORD dwBytesTransfered) override
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv got(%d)", dwBytesTransfered);
        auto pthis = this->pthis_;
        this->pthis_.reset();

        if (0 == dwBytesTransfered) {
          this->owner_->incoming_->write_async(this->sp_, nullptr, 0).wait(
            [pthis](const service_provider & sp, size_t) {
          },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & error) {
            sp.unhandled_exception(error);
          },
            this->sp_);
        }
        else {
          this->owner_->incoming_->write_all_async(this->sp_, this->buffer_, (size_t)dwBytesTransfered)
            .wait(
              [pthis](const service_provider & sp) {
                static_cast<_read_socket_task *>(pthis.get())->read_async();
              },
              [](const service_provider & sp, const std::shared_ptr<std::exception> & error) {
                sp.unhandled_exception(error);
              },
              this->sp_);
        }
      }
      void error(DWORD error_code) override
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSARecv error(%d)", error_code);
        if (ERROR_NETNAME_DELETED == error_code) {
          this->process(0);
        }
        else {
          this->sp_.unhandled_exception(std::make_shared<std::system_error>(error_code, std::system_category(), "read failed"));
        }
      }
    };

    class _write_socket_task : public _socket_task
    {
    public:
      constexpr static size_t BUFFER_SIZE = 1024;

      _write_socket_task(
        const service_provider & sp,
        const std::shared_ptr<_tcp_network_socket> & owner)
        : owner_(owner),
          sp_(sp)
      {
      }

      ~_write_socket_task()
      {
      }

      void write_async()
      {
        auto pthis = this->shared_from_this();
        this->owner_->outgoing_->read_async(this->sp_, this->buffer_, BUFFER_SIZE)
          .wait([pthis](const service_provider & sp, size_t len) {
              static_cast<_write_socket_task *>(pthis.get())->schedule(static_cast<_write_socket_task *>(pthis.get())->buffer_, len);
            },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            },
            this->sp_);
      }

      void check_timeout(const service_provider & sp) override
      {
      }

      void prepare_to_stop(const service_provider & sp) override
      {
      }

    private:
      service_provider sp_;
      std::shared_ptr<_tcp_network_socket> owner_;
      std::shared_ptr<_socket_task> pthis_;
      uint8_t buffer_[BUFFER_SIZE];

      void schedule(const void * data, size_t len)
      {
        if (0 == len) {
          shutdown(this->owner_->s_, SD_SEND);
          this->owner_.reset();
          return;
        }

        this->wsa_buf_.buf = (CHAR *)data;
        this->wsa_buf_.len = len;

        this->pthis_ = this->shared_from_this();

        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend");
        if (NOERROR != WSASend(this->owner_->s_, &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend error(%d)", errorCode);
            throw std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
      }

      void process(DWORD dwBytesTransfered) override
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "WSASend got(%d)", dwBytesTransfered);

        auto pthis = this->pthis_;
        this->pthis_.reset();

        try {
          if (this->wsa_buf_.len == dwBytesTransfered) {
            this->write_async();
          }
          else {
            this->schedule(this->wsa_buf_.buf + dwBytesTransfered, this->wsa_buf_.len - dwBytesTransfered);
          }
        }
        catch (const std::exception & ex) {
          this->sp_.unhandled_exception(std::make_shared<std::exception>(ex));
        }
        catch (...) {
          this->sp_.unhandled_exception(std::make_shared<std::runtime_error>("Unexpected error"));
        }
      }

      void error(DWORD error_code) override
      {
        this->sp_.unhandled_exception(std::make_shared<std::system_error>(error_code, std::system_category(), "write failed"));
      }
    };


#else
    class _socket_handler : public _socket_task
    {
      using this_class = _socket_handler;
    public:
      _socket_handler(
        const service_provider & sp,
        const std::shared_ptr<_tcp_network_socket> & owner)
        : sp_(sp), owner_(owner),
          network_service_(static_cast<_network_service *>(sp.get<inetwork_service>())),
          event_masks_(EPOLLIN | EPOLLET),
          read_timeout_ticks_(0),
          closed_(false)
      {
      }
      
      ~_socket_handler()
      {
        if(EPOLLET != this->event_masks_){
          this->network_service_->remove_association(this->sp_, this->owner_->s_);
        }
      }

      void start()
      {
        this->network_service_->associate(this->sp_, this->owner_->s_, this->shared_from_this(), this->event_masks_);
        this->schedule_write();
      }
      
      void prepare_to_stop(const service_provider & sp) override
      {
        if(!this->closed_){
          this->change_mask(0, EPOLLIN);

          this->sp_.get<logger>()->trace("TCP", this->sp_, "read timeout");
          this->closed_ = true;
          this->owner_->incoming_->write_all_async(this->sp_, nullptr, 0)
          .wait(
            [sp = this->sp_]() {
              sp.get<logger>()->trace("TCP", sp, "input closed");
            },
            [sp = this->sp_](const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            });
        }          
      }
 
      void process(uint32_t events) override
      {
        if(EPOLLOUT == (EPOLLOUT & events)){
          this->change_mask(0, EPOLLOUT);
          
          int len = write(
            this->owner_->s_,
            this->write_buffer_,
            this->write_len_);
          
          if (len < 0) {
            int error = errno;
            throw std::system_error(
              error,
              std::generic_category(),
              "Send");
          }
        
          if((size_t)len != this->write_len_){
            throw std::runtime_error("Invalid send TCP");
          }
          
          this->sp_.get<logger>()->trace("TCP", this->sp_, "Sent %d bytes", len);
          this->schedule_write();
        }
        
        if(EPOLLIN == (EPOLLIN & events)){
          this->change_mask(0, EPOLLIN);
          this->read_data();
        }
      }
      
      void check_timeout(const service_provider & sp) override
      {
        sp.get<logger>()->trace("TCP", sp, "check_timeout(ticks=%d, is_closed=%s)",
           this->read_timeout_ticks_,
           (this->closed_ ? "true" : "false"));
        
        if(1 < this->read_timeout_ticks_++ && !this->closed_){
          this->change_mask(0, EPOLLIN | EPOLLOUT);

          this->sp_.get<logger>()->trace("TCP", this->sp_, "read timeout");
          this->closed_ = true;
          this->owner_->incoming_->write_all_async(this->sp_, nullptr, 0)
          .wait(
            [sp = this->sp_](const service_provider & sp) {
              sp.get<logger>()->trace("TCP", sp, "input closed");
            },
            [sp = this->sp_](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            });
        }          
      }

    private:
      service_provider sp_;
      std::shared_ptr<_tcp_network_socket> owner_;
      _network_service * network_service_;
      
      uint8_t read_buffer_[10 * 1024 * 1024];
      
      uint8_t write_buffer_[10 * 1024 * 1024];
      size_t write_len_;
      std::mutex event_masks_mutex_;
      uint32_t event_masks_;
      int read_timeout_ticks_;
      bool closed_;
      
      void change_mask(uint32_t set_events, uint32_t clear_events = 0)
      {
        std::unique_lock<std::mutex> lock(this->event_masks_mutex_);
        auto need_create = (EPOLLET == this->event_masks_);
        this->event_masks_ |= set_events;
        this->event_masks_ &= ~clear_events;
        
        if(!need_create && EPOLLET != this->event_masks_){
          this->network_service_->set_events(this->sp_, this->owner_->s_, this->event_masks_);
        }
        else if (EPOLLET == this->event_masks_){
          this->network_service_->remove_association(this->sp_, this->owner_->s_);
        }
        else {
          this->network_service_->associate(this->sp_, this->owner_->s_, this->shared_from_this(), this->event_masks_);
        }
      }
     
      void schedule_write()
      {
        this->sp_.get<logger>()->trace("TCP", this->sp_, "waiting data to send");
        this->owner_->outgoing_->read_async(this->sp_, this->write_buffer_, sizeof(this->write_buffer_))
        .wait([pthis = this->shared_from_this()](size_t readed){
          if(0 == readed){
            //End of stream
            static_cast<this_class *>(pthis.get())->sp_.get<logger>()->trace("TCP",
              static_cast<this_class *>(pthis.get())->sp_, "output closed");
            shutdown(static_cast<this_class *>(pthis.get())->owner_->s_, SHUT_WR);
          }
          else {
            static_cast<this_class *>(pthis.get())->sp_.get<logger>()->trace("TCP", 
              static_cast<this_class *>(pthis.get())->sp_, "scheduled to send %d", readed);
            static_cast<this_class *>(pthis.get())->write_len_ = readed;
            static_cast<this_class *>(pthis.get())->change_mask(EPOLLOUT);
          }
        },
        [sp = this->sp_](const std::shared_ptr<std::exception> & ex){
          sp.get<logger>()->trace("TCP", sp, "%s at write", ex->what());
          sp.unhandled_exception(ex);
        });
      }

      void read_data()
      {
        int len = read(this->owner_->s_, this->read_buffer_, sizeof(this->read_buffer_));

        if (len < 0) {
          int error = errno;
          if(EAGAIN == error){
            this->change_mask(EPOLLIN);
            return;
          }
          
          throw std::system_error(error, std::system_category(), "recv");
        }
        
        if(0 == len){
          this->closed_ = true;
        }
        
        this->read_timeout_ticks_ = 0;
        this->sp_.get<logger>()->trace("TCP", this->sp_, "got %d bytes", len);
        this->owner_->incoming_->write_all_async(this->sp_, this->read_buffer_, len)
          .wait(
            [pthis = this->shared_from_this(), len](const service_provider & sp) {
              if(0 != len){
                static_cast<this_class *>(pthis.get())->read_data();
              }
              else {
                sp.get<logger>()->trace("TCP", sp, "input closed");
              }
            },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            },
            this->sp_);
      }
    };
#endif//_WIN32
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_P_H_
