#include <udp_socket.h>
#include "stdafx.h"
#include "chunk_manager.h"
#include "private/p2p_network_p.h"
#include "ip2p_network_client.h"

vds::p2p_network::p2p_network() {

}

vds::p2p_network::~p2p_network() {

}

void
vds::p2p_network::start(
    const vds::service_provider &sp,
    const std::shared_ptr<class ip2p_network_client> &client,
    const std::string &login,
    const std::string &password) {

  this->impl_.reset(new _p2p_network(client));
  this->impl_->start(sp, 0, login, password);

}

void
vds::p2p_network::start(
    const vds::service_provider &sp,
    const std::shared_ptr<class ip2p_network_client> &client,
    const vds::certificate &node_cert,
    const vds::asymmetric_private_key &node_key) {

  this->impl_.reset(new _p2p_network(client));
  this->impl_->start(sp, 0, node_cert, node_key);
}
//////////////////////////////////
vds::_p2p_network::_p2p_network(
    const std::shared_ptr<ip2p_network_client> &client)
: client_(client),
  backgroud_timer_("p2p network background") {
}

vds::_p2p_network::~_p2p_network() {

}

void vds::_p2p_network::start(
    const vds::service_provider &sp,
    int port,
    const std::string &login,
    const std::string &password) {

  this->start_network(sp, port);

}

void vds::_p2p_network::start(const vds::service_provider &sp, int port, const vds::certificate &node_cert,
                              const vds::asymmetric_private_key &node_key) {

  this->start_network(sp, port);
}

void vds::_p2p_network::start_network(const service_provider &sp, int port) {
  this->transport_.start(sp, port, [pthis = this->shared_from_this()](
      const udp_transport::session & source, const const_data_buffer & message) {
    pthis->handle_incoming_message(source, message);
  });

  this->do_backgroud_tasks(sp);

  this->backgroud_timer_.start(sp, std::chrono::seconds(5), [sp, pthis = this->shared_from_this()]()->bool{
    return pthis->do_backgroud_tasks(sp);
  });
}

bool vds::_p2p_network::do_backgroud_tasks(const service_provider &sp) {
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
                  this->transport_.connect(sp, address);
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
                return true;
              });
        }
      });
  return !sp.get_shutdown_event().is_shuting_down();
}

void vds::_p2p_network::handle_incoming_message(
    const udp_transport::session &source,
    const const_data_buffer &message) {
}

