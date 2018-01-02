#ifndef __VDS_P2P_NETWORK_UDP_TRANSPORT_H_
#define __VDS_P2P_NETWORK_UDP_TRANSPORT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "async_buffer.h"
#include "leak_detect.h"
#include "leak_detect.h"

namespace vds {

  class udp_transport {
  public:
    static constexpr uint32_t protocol_version = 0;

    class _session : public std::enable_shared_from_this<_session>
    {
    public:
      _session(){
        this->leak_detect_.name_ = "vds.udp_transport._session";
        this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector){
          this->dump(collector);
        };
      }
      virtual ~_session() {}

      virtual void send(
          const service_provider &sp,
          const const_data_buffer &message) = 0;

      virtual async_task<const const_data_buffer &> read_async(
          const service_provider & sp) = 0;

      virtual void close(
          const service_provider & sp,
          const std::shared_ptr<std::exception> & ex) = 0;

      virtual async_task<> prepare_to_stop(const vds::service_provider &sp) = 0;

      leak_detect_helper leak_detect_;

      virtual void dump(leak_detect_collector * collector) = 0;
    };

    class session {
    public:
      session(const session & other) = default;

      session(const std::shared_ptr<_session> &impl)
      : impl_(impl){
      }

      void send(
          const service_provider & sp,
          const const_data_buffer & message){
        this->impl_->send(sp, message);
      }

      async_task<const const_data_buffer &> read_async(const service_provider & sp){
        return this->impl_->read_async(sp);
      }

      _session * operator -> () const { return this->impl_.get(); }

      async_task<> prepare_to_stop(const vds::service_provider &sp);

      operator bool () const {
        return this->impl_.get() != nullptr;
      }

    protected:
      std::shared_ptr<_session> impl_;
    };

    typedef std::function<void(const session & session)> new_session_handler_t;

    udp_transport();
	  ~udp_transport();

    void start(
        const vds::service_provider &sp,
        int port,
        const new_session_handler_t &new_session_handler);

    void stop(const service_provider & sp);

    void connect(
        const service_provider & sp,
        const std::string & address);

    async_task<> prepare_to_stop(const service_provider &sp);

    class _udp_transport *operator -> () const {
      return this->impl_.get();
    }
    operator bool () const {
      return nullptr != this->impl_.get();
    }
  private:
    std::shared_ptr<class _udp_transport> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_UDP_TRANSPORT_H_
