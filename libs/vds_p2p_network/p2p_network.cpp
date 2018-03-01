#include "stdafx.h"
#include "p2p_network.h"
#include "private/p2p_network_p.h"
#include "ip2p_network_client.h"
#include "udp_socket.h"
#include "private/p2p_crypto_tunnel_p.h"
#include "private/p2p_route_p.h"
#include "run_configuration_dbo.h"
#include "p2p_network_service.h"
#include "private/p2p_network_service_p.h"
#include "messages/p2p_message_id.h"
#include "messages/channel_log_state.h"
#include "../vds_log_sync/log_sync_service.h"
#include "messages/channel_log_request.h"
#include "user_manager.h"
#include "cert_control.h"
#include "certificate_chain_dbo.h"
#include "messages/chunk_send_replica.h"
#include "chunk_manager.h"
#include "chunk_replicator.h"
#include "messages/chunk_query_replica.h"
#include "messages/chunk_offer_replica.h"
#include "messages/chunk_have_replica.h"
#include "private/p2p_route_p.h"

vds::p2p_network::p2p_network()
:impl_(new _p2p_network()){
}

vds::p2p_network::~p2p_network() {

}

vds::async_task<> vds::p2p_network::start_network(const vds::service_provider &sp) {
  return this->impl_->start_network(sp);
}

vds::async_task<> vds::p2p_network::prepare_to_stop(const vds::service_provider &sp) {
  return this->impl_->prepare_to_stop(sp);
}

void vds::p2p_network::stop(const vds::service_provider &sp) {
  this->impl_->stop(sp);
  this->impl_.reset();
}

void vds::p2p_network::send(
    const vds::service_provider &sp,
    const vds::guid &device_id,
    const vds::const_data_buffer &message) {
  this->impl_->send(sp, device_id, message);
}

bool vds::p2p_network::send_tentatively(const vds::service_provider &sp, const vds::guid &device_id,
                                        const vds::const_data_buffer &message, size_t distance) {
  return this->impl_->send_tentatively(sp, device_id, message, distance);

}

vds::guid vds::p2p_network::current_node_id() const {
  return this->impl_->current_node_id();
}


//////////////////////////////////
vds::_p2p_network::_p2p_network()
{
}

vds::_p2p_network::~_p2p_network() {

}


struct run_data
{
  vds::guid id;
  int port;
  vds::asymmetric_private_key key;
  std::list<vds::certificate> cert_chain;
};


vds::async_task<> vds::_p2p_network::start_network(const vds::service_provider &parent_scope) {
  auto sp = parent_scope.create_scope(__FUNCTION__);
  imt_service::enable_async(sp);

  auto run_conf = std::make_shared<run_data>();
  return sp.get<db_model>()->async_transaction(sp, [sp, run_conf, pthis = this->shared_from_this()](
      database_transaction &t) {

    auto user_mng = sp.get<user_manager>();

    dbo::run_configuration t1;
    auto st = t.get_reader(t1.select(t1.id, t1.port, t1.cert, t1.cert_private_key));
    if(!st.execute()) {
      throw std::runtime_error("There is no active network configuration");
    }

    auto device_cert = certificate::parse_der(t1.cert.get(st));

    run_conf->id = t1.id.get(st);
    run_conf->port = t1.port.get(st);
    run_conf->key = asymmetric_private_key::parse_der(t1.cert_private_key.get(st), std::string());
    run_conf->cert_chain.push_back(device_cert);

    if(st.execute()) {
      throw std::runtime_error("Multiple run configuration is not found");
    }

    user_mng->load(sp, t, run_conf->id);

    auto parent_id = cert_control::get_parent_id(run_conf->cert_chain.front());
    while (parent_id) {
      dbo::certificate_chain_dbo t2;
      auto st = t.get_reader(t2.select(t2.cert).where(t2.id == parent_id));
      if (!st.execute()) {
        throw std::runtime_error("Wrong certificate id " + parent_id.str());
      }

      auto cert = certificate::parse_der(t2.cert.get(st));
      run_conf->cert_chain.push_front(cert);
      parent_id = cert_control::get_parent_id(cert);
    }

    return true;
  }).then([sp, run_conf, pthis = this->shared_from_this()]() {
    return pthis->network_service_.start(sp, run_conf->port, run_conf->cert_chain, run_conf->key);
  }).then([sp, pthis = this->shared_from_this()]() {
    pthis->route_->start(sp);
  });
}

vds::async_task<> vds::_p2p_network::prepare_to_stop(const vds::service_provider &sp) {
  return this->network_service_.prepare_to_stop(sp);
}

void vds::_p2p_network::stop(const vds::service_provider &sp) {
  this->network_service_.stop(sp);
}

bool vds::_p2p_network::send(
    const vds::service_provider &sp,
    const node_id_t & node_id,
    const vds::const_data_buffer &message) {
  return this->route_->send(
      sp,
      node_id,
      message);
}

void vds::_p2p_network::process_input_command(
  const service_provider &sp,
  const guid &partner_id,
  const std::shared_ptr<_p2p_crypto_tunnel> &session,
  const const_data_buffer &message_data) {

  binary_deserializer ms(message_data);
  node_id_t target_node;
  const_data_buffer message_buffer;
  ms >> target_node >> message_buffer;

  if(target_node.device_id() != this->current_node_id()){
    this->send(sp, target_node, message_buffer);
    return;
  }

  uint8_t  command_id;
  binary_deserializer s(message_buffer);
  s >> command_id;

  switch ((p2p_messages::p2p_message_id)command_id) {
  case p2p_messages::p2p_message_id::channel_log_state: {
    p2p_messages::channel_log_state message(s);
    sp.get<log_sync_service>()->apply(sp, partner_id, message);

    break;
  }
  case p2p_messages::p2p_message_id::channel_log_request: {
    p2p_messages::channel_log_request message(s);
    sp.get<log_sync_service>()->apply(sp, partner_id, message);

    break;
  }
  case p2p_messages::p2p_message_id::channel_log_record: {
    p2p_messages::channel_log_record message(s);
    sp.get<log_sync_service>()->apply(sp, partner_id, message);

    break;
  }
  case p2p_messages::p2p_message_id::chunk_send_replica: {
    p2p_messages::chunk_send_replica message(s);

    sp.get<chunk_replicator>()->apply(sp, partner_id, message);

    break;
  }
  case p2p_messages::p2p_message_id::chunk_query_replica: {
    p2p_messages::chunk_query_replica message(s);
    sp.get<chunk_replicator>()->apply(sp, partner_id, message);
    break;
  }
    case p2p_messages::p2p_message_id::chunk_offer_replica: {
      p2p_messages::chunk_offer_replica message(s);
      sp.get<chunk_replicator>()->apply(sp, partner_id, message);
      break;
    }

    case p2p_messages::p2p_message_id::chunk_have_replica: {
      p2p_messages::chunk_have_replica message(s);
      sp.get<chunk_replicator>()->apply(sp, partner_id, message);
      break;
    }

  default:
    throw std::runtime_error("Invalid command");
  }
}

void vds::_p2p_network::close_session(
    const vds::service_provider &sp,
    const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session) {
  this->route_->close_session(sp, proxy_session);
}

void vds::_p2p_network::add_node(
    const vds::service_provider &sp,
    const vds::node_id_t &id,
    const std::shared_ptr<vds::_p2p_crypto_tunnel> &proxy_session) {
  this->route_->add_node(sp, id, proxy_session);

}

bool vds::_p2p_network::send_tentatively(const vds::service_provider &sp, const vds::guid &device_id,
                                         const vds::const_data_buffer &message, size_t distance) {
  std::set<node_id_t> candidates;
  this->route_->search_nodes(sp, device_id, distance, candidates);

  if(!candidates.empty()) {
    size_t index = std::rand() % candidates.size();
    for (auto &p : candidates) {
      if (0 == index--) {
        this->route_->send(
            sp,
            p,
            message);
        return true;
      }
    }
  }
  else {
    sp.get<logger>()->warning(
        ThisModule,
        sp,
        "No canditates to route %s",
        device_id.str().c_str());
  }

  return false;
}

vds::guid vds::_p2p_network::current_node_id() const {
  return this->route_->current_node_id().device_id();
}

