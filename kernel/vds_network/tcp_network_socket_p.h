#ifndef __VDS_NETWORK_NETWORK_SOCKET_P_H_
#define __VDS_NETWORK_NETWORK_SOCKET_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_types_p.h"
#include "tcp_network_socket.h"
#include "socket_task_p.h"

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
        this->network_service_->remove(this->s_);
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
#ifdef _WIN32
      std::make_shared<_read_socket_task>(sp, this->shared_from_this())->read_async();
      std::make_shared<_write_socket_task>(sp, this->shared_from_this())->write_async();
#else
#endif//_WIN32
    }

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
      }

      void read_async()
      {
        this->wsa_buf_.len = BUFFER_SIZE;
        this->wsa_buf_.buf = (CHAR *)this->buffer_;

        this->pthis_ = this->shared_from_this();

        DWORD flags = 0;
        DWORD numberOfBytesRecvd;
        if (NOERROR != WSARecv(this->owner_->s_, &this->wsa_buf_, 1, &numberOfBytesRecvd, &flags, &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
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

    private:
      service_provider sp_;
      std::shared_ptr<_tcp_network_socket> owner_;
      std::shared_ptr<_socket_task> pthis_;
      uint8_t buffer_[BUFFER_SIZE];

      void process(DWORD dwBytesTransfered) override
      {
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
        this->sp_.unhandled_exception(std::make_shared<std::system_error>(error_code, std::system_category(), "read failed"));
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

        if (NOERROR != WSASend(this->owner_->s_, &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            throw std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
      }

      void process(DWORD dwBytesTransfered) override
      {
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
    class _socket_tasks : public _socket_task
    {
    public:
      void process(uint32_t signals) override
      {
        try {
          if (EPOLLIN == (EPOLLIN & signals)) {
            int len = read(this->s_, this->buffer_, this->buffer_size_);
            if (len < 0) {
              int error = errno;
              throw
                std::system_error(
                  error,
                  std::system_category());
            }

            imt_service::async(this->sp_, [this, len]() {
              this->readed_method_(this->sp_, len);
            });
          }
          if (EPOLLOUT == (EPOLLOUT & signals)) {
            logger::get(this->sp_)->trace(this->sp_, "write %d bytes", this->data_size_);
            try {
              int len = write(this->s_, this->data_, this->data_size_);
              if (len < 0) {
                int error = errno;
                throw std::system_error(
                  error,
                  std::system_category());
              }

              imt_service::async(this->sp_,
                [this, len]() {
                this->written_method_(this->sp_, len);
              });
            }
            catch (const std::exception & ex) {
              this->error_method_(this->sp_, std::make_shared<std::exception>(ex));
            }
            catch (...) {
              this->error_method_(this->sp_, std::make_shared<std::runtime_error>("Unexpected error"));
            }
          }

        }
        catch (const std::exception & ex) {
          this->error_method_(this->sp_, std::make_shared<std::exception>(ex));
        }
        catch (...) {
          this->error_method_(this->sp_, std::make_shared<std::runtime_error>("Unexpected error"));
        }
      }

    };

#endif//_WIN32
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_P_H_