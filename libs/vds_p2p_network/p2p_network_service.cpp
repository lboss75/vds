//
// Created by vadim on 30.10.17.
//

#include "p2p_network_service.h"
#include "private/p2p_network_service_p.h"
#include "ip2p_network_client.h"
#include "stdafx.h"
#include "private/p2p_crypto_tunnel_with_login_p.h"
#include "private/p2p_crypto_tunnel_with_certificate_p.h"
#include "private/udp_transport_p.h"

vds::p2p_network_service::p2p_network_service()
{
}

vds::p2p_network_service::~p2p_network_service()
{
}

vds::async_task<> vds::p2p_network_service::start(
    const vds::service_provider &sp,
    int port,
    const std::list<certificate> &certificate_chain,
    const vds::asymmetric_private_key &node_key) {
  this->impl_.reset(new _p2p_network_service(sp));
  return this->impl_->start(sp, port, certificate_chain, node_key);

}

vds::async_task<> vds::p2p_network_service::start(
    const vds::service_provider &sp,
    const std::string &device_name,
    int port,
    const std::string &login,
    const std::string &password) {
  this->impl_.reset(new _p2p_network_service(sp));
  return this->impl_->start(sp, device_name, port, login, password);
}

vds::async_task<> vds::p2p_network_service::prepare_to_stop(const vds::service_provider &sp) {
  return this->impl_->prepare_to_stop(sp);
}

void vds::p2p_network_service::stop(const vds::service_provider & sp) {
  this->impl_->stop(sp);
  this->impl_.reset();
}

//////////////////////////
vds::_p2p_network_service::_p2p_network_service(const vds::service_provider &sp)
: backgroud_timer_("p2p network background") {
  this->leak_detect_.name_ = "_p2p_network_service";
  this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector){
    collector->add(this->transport_);
    for(auto & s : this->sessions_) {
      collector->add(s);
    }
  };
}

vds::async_task<>
vds::_p2p_network_service::start(
    const vds::service_provider &sp,
    const std::string &device_name,
    int port,
    const std::string &login,
    const std::string &password) {
  return [sp, pthis = this->shared_from_this(), device_name, port, login, password](const async_result<> & start_result){
    pthis->start_network(
        sp,
        port,
        [pthis, sp, start_result, device_name, port, login, password](const udp_transport::session & session){
          p2p_crypto_tunnel tunnel(std::make_shared<_p2p_crypto_tunnel_with_login>(
              session,
              start_result,
              login,
              password,
              device_name,
              port));
          tunnel.start(sp);

          std::unique_lock<std::shared_mutex> lock(pthis->sessions_mutex_);
          pthis->sessions_.push_back(tunnel);
        }
    );
  };
}

vds::async_task<> vds::_p2p_network_service::start(const vds::service_provider &sp, int port,
                                                   const std::list<certificate> &certificate_chain,
                                                   const asymmetric_private_key &node_key) {
  this->start_network(sp, port,
                      [pthis = this->shared_from_this(), sp, certificate_chain, node_key](const udp_transport::session & session){
                        p2p_crypto_tunnel tunnel(std::make_shared<_p2p_crypto_tunnel_with_certificate>(
                            session, certificate_chain, node_key));
                        tunnel.start(sp);

                        std::unique_lock<std::shared_mutex> lock(pthis->sessions_mutex_);
                        pthis->sessions_.push_back(tunnel);
                      });

  return vds::async_task<>::empty();
}

void vds::_p2p_network_service::start_network(const service_provider &sp, int port,
                                      const udp_transport::new_session_handler_t &new_session_handler) {
  this->transport_.start(sp, port, new_session_handler);

  this->do_backgroud_tasks(sp);

  this->backgroud_timer_.start(sp, std::chrono::seconds(5), [sp, pthis = this->shared_from_this()]()->bool{
    return pthis->do_backgroud_tasks(sp);
  });
}

bool vds::_p2p_network_service::do_backgroud_tasks(const service_provider &sp) {
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

vds::async_task<> vds::_p2p_network_service::prepare_to_stop(const vds::service_provider &sp) {
  return [pthis = this->shared_from_this(), sp](const async_result<> & result){
    std::unique_lock<std::shared_mutex> lock(pthis->sessions_mutex_);
    auto runner = new _async_series(result, 1 + pthis->sessions_.size());
    runner->add(pthis->transport_.prepare_to_stop(sp));

    for(auto & s : pthis->sessions_){
      runner->add(s.prepare_to_stop(sp));
    }
  };
}

void vds::_p2p_network_service::stop(const vds::service_provider &sp) {
  this->transport_.stop(sp);
  this->sessions_.clear();

}
