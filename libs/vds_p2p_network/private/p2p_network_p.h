#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_P_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include "udp_transport.h"
#include "ip2p_network_storage.h"
#include "url_parser.h"
#include "db_model.h"
#include "well_known_node_dbo.h"

namespace vds {

  class _p2p_network : public std::enable_shared_from_this<_p2p_network> {
  public:
    _p2p_network(
        const std::shared_ptr<ip2p_network_storage> &storage,
        const std::shared_ptr<ip2p_network_client> &client);

    ~_p2p_network();

    void start(
        const vds::service_provider &sp,
        const std::string &login,
        const std::string &password);

    void start(
        const vds::service_provider &sp,
        const vds::certificate &node_cert,
        const vds::asymmetric_private_key &node_key);

  private:
    std::shared_ptr<ip2p_network_storage> storage_;
    std::shared_ptr<ip2p_network_client> client_;

    udp_transport udp_transport_;
    timer backgroud_timer_;

    void start_network(const service_provider &sp);

    bool do_backgroud_tasks(const service_provider &sp);
    void handle_incoming_message(
        const udp_transport::session & source,
        const const_data_buffer & message);
  };

  _p2p_network::_p2p_network(const std::shared_ptr<ip2p_network_storage> &storage,
                             const std::shared_ptr<ip2p_network_client> &client)
  : storage_(storage), client_(client), udp_transport_([pthis = this->shared_from_this()](
      const udp_transport::session & source, const const_data_buffer & message) {
    pthis->handle_incoming_message(source, message);
  }),
    backgroud_timer_("p2p network background") {
  }


  void _p2p_network::start(
      const vds::service_provider &sp,
      const std::string &login,
      const std::string &password) {

    this->start_network(sp);

  }

  void _p2p_network::start(
      const vds::service_provider &sp,
      const vds::certificate &node_cert,
      const vds::asymmetric_private_key &node_key) {

    this->start_network(sp);

  }

  void _p2p_network::start_network(const service_provider & sp) {
    this->udp_transport_.start(sp);
    this->do_backgroud_tasks(sp);

    this->backgroud_timer_.start(sp, std::chrono::seconds(5), [sp, pthis = this->shared_from_this()]()->bool{
      return pthis->do_backgroud_tasks(sp);
    });
  }

  bool _p2p_network::do_backgroud_tasks(const service_provider &sp) {
    auto scope = sp.create_scope("P2P timer job");
    mt_service::enable_async(scope);
    sp.get<db_model>()->async_transaction(
        scope,
        [this, scope](database_transaction & t) {

          well_known_node_dbo t1;

          auto st = t.get_reader(t1.select(t1.addresses));
          while (st.execute()) {
            url_parser::parse_addresses(
                t1.addresses.get(st),
                [this, scope](const std::string &protocol, const std::string &address) -> bool {
                  auto sp = scope.create_scope(("Connecting to " + address).c_str());

                  if ("udp" == protocol) {
                    this->udp_transport_.connect(sp, address);
                  } else if ("https" == protocol) {
                    //            auto na = url_parser::parse_network_address(address);
                    //            this->start_https_server(scope, na)
                    //                .execute(
                    //                    [this, sp = scope](const std::shared_ptr<std::exception> & ex) {
                    //                      if(!ex){
                    //                        sp.get<logger>()->info("HTTPS", sp, "Servers stopped");
                    //                      } else {
                    //                        sp.get<logger>()->info("HTTPS", sp, "Server error: %s", ex->what());
                    //                      }});
                  }
                });
          }
        });
    return !sp.get_shutdown_event().is_shuting_down();
  }
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_P_H_
