#ifndef __VDS_P2P_NETWORK_UDP_TRANSPORT_P_H_
#define __VDS_P2P_NETWORK_UDP_TRANSPORT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include <queue>

#include "async_task.h"
#include "udp_socket.h"
#include "resizable_data_buffer.h"
#include "udp_transport_queue_p.h"
#include "udp_transport.h"
#include "thread_apartment.h"

namespace vds {

  class _udp_transport : public std::enable_shared_from_this<_udp_transport> {
  public:
    _udp_transport(const udp_transport::new_session_handler_t & new_session_handler);
    ~_udp_transport();   

    void start(const service_provider &sp, int port);
    void stop(const service_provider & sp);

    void connect(const service_provider & sp, const std::string & address);
    void send_broadcast(int port);

    const std::shared_ptr<_udp_transport_queue> & send_queue() const {
      return this->send_queue_;
    }

    async_task<> prepare_to_stop(const vds::service_provider &sp);

  private:
    friend class _udp_transport_queue;
    friend class _udp_transport_session;

    udp_server server_;
    guid instance_id_;
    udp_transport::new_session_handler_t new_session_handler_;

    udp_datagram incomming_buffer_;
    bool incomming_eof_;

    std::map<_udp_transport_session_address_t, std::shared_ptr<_udp_transport_session>> sessions_;
    std::shared_mutex sessions_mutex_;
	bool is_closed_;

    std::shared_ptr<_udp_transport_queue> send_queue_;

    timer timer_;

    bool do_timer_tasks(
        const service_provider &sp);
    const_data_buffer create_handshake_message();

    void process_incommig_message(
        const service_provider &sp,
        const udp_datagram &message);

    void continue_read_socket(const service_provider & sp);

    void send_data(
        const service_provider & sp,
        const std::shared_ptr<_udp_transport_session> & session,
        const const_data_buffer & data);

    async_task<> write_async(udp_datagram && message);

    void read_message(const service_provider &sp);

    void handshake_completed(
        const service_provider &sp,
        _udp_transport_session * session);
  };
}

#endif //__VDS_P2P_NETWORK_UDP_TRANSPORT_P_H_
