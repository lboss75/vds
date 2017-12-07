#include <udp_socket.h>
#include "stdafx.h"
#include "p2p_network.h"
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
  backgroud_timer_("p2p network background"),
  start_result_([](const std::shared_ptr<std::exception> & ex){
    throw std::runtime_error("Login error");
  }){
}

vds::_p2p_network::~_p2p_network() {

}

vds::async_task<> vds::_p2p_network::start(
    const vds::service_provider &sp,
    int port,
    const std::string &login,
    const std::string &password) {

  return [sp, pthis = this->shared_from_this(), port, login, password](const async_result<> & start_result){
    pthis->start_result_ = start_result;
    pthis->start_network(sp, port,
      [pthis, login, password](const udp_transport::session & session){
        p2p_crypto_tunnel tunnel;
        tunnel.start(session, login, password);

        std::unique_lock<std::shared_mutex> lock(pthis->sessions_mutex_);
        pthis->sessions_.push_back(tunnel);
    });
  };
}

vds::async_task<> vds::_p2p_network::start(
    const vds::service_provider &sp,
    int port,
    const vds::certificate &node_cert,
    const vds::asymmetric_private_key &node_key) {

  this->start_network(sp, port,
  [pthis = this->shared_from_this(), node_cert, node_key](const udp_transport::session & session){
    p2p_crypto_tunnel tunnel;
    tunnel.start(session, node_cert, node_key);

    std::unique_lock<std::shared_mutex> lock(pthis->sessions_mutex_);
    pthis->sessions_.push_back(tunnel);
  });

  return vds::async_task<>::empty();
}

void vds::_p2p_network::start_network(const service_provider &sp, int port,
                                      const udp_transport::new_session_handler_t &new_session_handler) {
  this->transport_.start(sp, port, new_session_handler);

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
      })
      .execute([sp](const std::shared_ptr<std::exception> & ex){
        if(ex) {
          sp.unhandled_exception(ex);
        }
      });
  return !sp.get_shutdown_event().is_shuting_down();
}

