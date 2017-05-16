#ifndef __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_
#define __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "write_socket_task.h"

namespace vds {
  class output_network_stream
  {
  public:
    output_network_stream(
      const network_socket & s)
      : s_(s)
    {
    }

    using incoming_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler
      : public socket_task,
        public vds::sync_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = vds::sync_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const output_network_stream & args)
      : base_class(context),
        s_(args.s_.handle())
      {
      }
      
      size_t sync_push_data(
        const vds::service_provider & sp)
      {        
#ifdef _DEBUG
        if (this->is_scheduled_) {
          throw std::exception();
        }
        this->is_scheduled_ = true;
#endif
#ifdef _WIN32
        this->wsa_buf_.len = (ULONG)this->data_size_;
        this->wsa_buf_.buf = (CHAR *)this->data_;

        if (NOERROR != WSASend(this->s_, &this->wsa_buf_, 1, NULL, 0, &this->overlapped_, NULL)) {
          auto errorCode = WSAGetLastError();
          if (WSA_IO_PENDING != errorCode) {
            this->is_scheduled_ = false;
            throw std::system_error(errorCode, std::system_category(), "WSASend failed");
          }
        }
#else
        if (nullptr == this->event_) {
          this->event_ = event_new(
            static_cast<network_service *>(sp.get<inetwork_manager>())->base_,
            this->s_,
            EV_WRITE,
            &handler::callback,
            this);
        }
        // Schedule client event
        event_add(this->event_, NULL);

        static_cast<network_service *>(sp.get<inetwork_manager>())->start_libevent_dispatch(sp);

#endif// _WIN32
      }
      
    private:
      network_socket::SOCKET_HANDLE s_;

#ifdef _WIN32
      void process(DWORD dwBytesTransfered) override
      {
        try {
#ifdef _DEBUG
          if (!this->is_scheduled_) {
            throw std::exception();
          }
          this->is_scheduled_ = false;
#endif
          this->data_ += dwBytesTransfered;
          this->data_size_ -= dwBytesTransfered;

          if (this->data_size_ > 0) {
            this->schedule(this->sp_);
          }
          else {
            this->done_method_(this->sp_);
          }
        }
        catch (...) {
          this->error_method_(this->sp_, std::current_exception());
        }
      }
#else

      static void callback(int fd, short event, void *arg)
      {
        auto pthis = reinterpret_cast<handler *>(arg);
        logger::get(pthis->sp_)->trace(pthis->sp_, "write %d bytes", pthis->data_size_);
        try {
          int len = write(fd, pthis->data_, pthis->data_size_);
          if (len < 0) {
            int error = errno;
            throw std::system_error(
              error,
              std::system_category());
          }

          pthis->data_ += len;
          pthis->data_size_ -= len;
          if (0 < pthis->data_size_) {
            //event_set(&pthis->event_, pthis->s_, EV_WRITE, &write_socket_task::callback, pthis);
            event_add(pthis->event_, NULL);
          }
          else {
            imt_service::async(pthis->sp_,
              [pthis]() {
              pthis->done_method_(pthis->sp_);
            });
          }
        }
        catch (...) {
          pthis->error_method_(pthis->sp_, std::current_exception());
        }
      }
#endif// _WIN32
    };
    
  private:
    const network_socket & s_;
  };
}

#endif // __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_
