//#ifndef __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
//#define __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "read_socket_task.h"
//#include "pipeline_filter.h"
//#include "network_manager.h"
//
//namespace vds {
//  class input_network_stream
//  {
//  public:
//    input_network_stream(
//      const network_socket & s)
//      : s_(s)
//    {
//    }
//    
//    using outgoing_item_type = uint8_t;
//    static constexpr size_t BUFFER_SIZE = 1024;
//    static constexpr size_t MIN_BUFFER_SIZE = 10;
//
//    template <typename context_type>
//    class handler 
//      : public socket_task,
//        public async_dataflow_source<context_type, handler<context_type>>
//    {
//      using base = async_dataflow_source<context_type, handler<context_type>>;
//    public:
//      handler(
//        const context_type & context,
//        const input_network_stream & args)
//      : base(context),
//        sp_(service_provider::empty()),
//        s_(args.s_.handle())
//#ifdef _DEBUG
//        , is_scheduled_(false)
//#endif // _DEBUG
//      {
//      }
//
//      ~handler()
//      {
//#ifdef _DEBUG
//        if (this->is_scheduled_) {
//          throw std::runtime_error("");
//        }
//#endif // _DEBUG
//      }
//
//      void async_get_data()
//      {
//        this->sp_ = sp;
//
//#ifdef _WIN32
//        this->wsa_buf_.len = this->input_buffer_size_;
//        this->wsa_buf_.buf = (CHAR *)this->input_buffer_;
//
//#ifdef _DEBUG
//        if (this->is_scheduled_) {
//          throw std::exception();
//        }
//        this->is_scheduled_ = true;
//#endif
//        DWORD flags = 0;
//        DWORD numberOfBytesRecvd;
//        if (NOERROR != WSARecv(this->s_, &this->wsa_buf_, 1, &numberOfBytesRecvd, &flags, &this->overlapped_, NULL)) {
//          auto errorCode = WSAGetLastError();
//          if (WSA_IO_PENDING != errorCode) {
//            this->is_scheduled_ = false;
//            throw std::system_error(errorCode, std::system_category(), "WSARecv failed");
//          }
//        }
//#else//!_WIN32
//        if (nullptr == this->event_) {
//          this->event_ = event_new(
//            static_cast<network_service *>(sp->get<inetwork_manager>())->base_,
//            this->s_,
//            EV_READ,
//            &handler::callback,
//            this);
//        }
//        // Schedule client event
//        event_add(this->event_, NULL);
//        static_cast<network_service *>(sp->get<inetwork_manager>())->start_libevent_dispatch(sp);
//#endif//_WIN32
//      }
//
//    private:
//      service_provider sp_;
//      network_socket::SOCKET_HANDLE s_;
//#ifdef _DEBUG
//      bool is_scheduled_;
//#endif
//
//#ifdef _WIN32
//      void process(DWORD dwBytesTransfered) override
//      {
//#ifdef _DEBUG
//        if (!this->is_scheduled_) {
//          throw std::exception();
//        }
//        this->is_scheduled_ = false;
//#endif
//        this->push_data((size_t)dwBytesTransfered);
//      }
//#else//!_WIN32
//      static void callback(int fd, short event, void *arg)
//      {
//        auto pthis = reinterpret_cast<handler *>(arg);
//        try {
//          int len = read(fd, pthis->buffer_, BUFFER_SIZE);
//          if (len < 0) {
//            int error = errno;
//            throw
//              std::system_error(
//                error,
//                std::system_category());
//          }
//
//          logger::get(pthis->sp_)->trace(pthis->sp_, "Receive %d bytes", len);
//          imt_service::async(pthis->sp_, [pthis, len]() {
//            pthis->next_method_(pthis->sp_, pthis->buffer_, len);
//          });
//        }
//        catch (...) {
//          pthis->error_method_(pthis->sp_, std::current_exception());
//        }
//      }
//#endif//_WIN32
//
//    };
//  private:
//    const network_socket & s_;
//  };
//}
//
//#endif // __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
