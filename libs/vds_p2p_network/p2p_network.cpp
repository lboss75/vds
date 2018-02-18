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
#include "messages/common_log_state.h"
#include "../vds_log_sync/log_sync_service.h"
#include "messages/common_block_request.h"
#include "user_manager.h"
#include "cert_control.h"
#include "certificate_chain_dbo.h"
#include "messages/chunk_send_replica.h"
#include "chunk_manager.h"
#include "chunk_replicator.h"
#include "messages/chunk_query_replica.h"
#include "messages/chunk_offer_replica.h"
#include "messages/chunk_have_replica.h"

vds::p2p_network::p2p_network()
:impl_(new _p2p_network()){
}

vds::p2p_network::~p2p_network() {

}

void
vds::p2p_network::random_broadcast(const vds::service_provider &sp, const vds::const_data_buffer &message) {
  this->impl_->random_broadcast(sp, message);
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

std::set<vds::p2p::p2p_node_info> vds::p2p_network::get_neighbors() const {
  return this->impl_->get_neighbors();
}

void vds::p2p_network::broadcast(const vds::service_provider &sp, const vds::const_data_buffer &message) {
  this->impl_->broadcast(sp, message);
}

void vds::p2p_network::send(const vds::service_provider &sp, const vds::guid &device_id,
                            const vds::const_data_buffer &message) {
  this->impl_->send(sp, device_id, message);
}

void vds::p2p_network::close_session(
    const service_provider &sp,
    const guid &partner,
    const std::shared_ptr<std::exception> & ex) {
  this->impl_->close_session(sp, partner, ex);
}

void vds::p2p_network::query_replica(
    const service_provider &sp,
    const const_data_buffer & data_hash)
{
	this->impl_->query_replica(sp, data_hash);
}


//////////////////////////////////
vds::_p2p_network::_p2p_network()
{
  this->leak_detect_.name_ = "_p2p_network";
  this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector){
    for(auto & p : this->network_services_){
      collector->add(p);
    }
  };
}

vds::_p2p_network::~_p2p_network() {

}

void
vds::_p2p_network::random_broadcast(
    const vds::service_provider &sp,
    const vds::const_data_buffer &message) {
  this->route_.random_broadcast(sp, message);
}

void vds::_p2p_network::add_route(
    const service_provider &sp,
    const vds::guid &partner_id,
    const std::shared_ptr<_p2p_crypto_tunnel> &session) {
  this->route_->add(sp, partner_id, session);
}

struct run_data
{
  vds::guid id;
  int port;
  vds::asymmetric_private_key key;
  vds::service_provider scope;
  std::list<vds::certificate> cert_chain;
};


vds::async_task<> vds::_p2p_network::start_network(const vds::service_provider &parent_scope) {
	auto sp = parent_scope.create_scope(__FUNCTION__);
	imt_service::enable_async(sp);

  auto run_conf = std::make_shared<std::list<run_data>>();
  return sp.get<db_model>()->async_transaction(sp, [sp, run_conf, pthis = this->shared_from_this()](database_transaction & t){

    auto user_mng = sp.get<user_manager>();

    dbo::run_configuration t1;
    auto st = t.get_reader(t1.select(t1.id, t1.port, t1.cert, t1.cert_private_key));
    while(st.execute()){
      auto device_cert = certificate::parse_der(t1.cert.get(st));

      run_conf->push_back(run_data{
        t1.id.get(st),
        t1.port.get(st),
        asymmetric_private_key::parse_der(t1.cert_private_key.get(st), std::string()),
        sp.create_scope("Configuration " + t1.id.get(st).str()) });

      run_conf->rbegin()->cert_chain.push_back(device_cert);
      user_mng->load(run_conf->rbegin()->scope, t, t1.id.get(st));
    }
	for (auto & conf : *run_conf) {
		auto parent_id = cert_control::get_parent_id(conf.cert_chain.front());
		while (parent_id) {
			dbo::certificate_chain_dbo t2;
			auto st = t.get_reader(t2.select(t2.cert).where(t2.id == parent_id));
			if (!st.execute()) {
				throw std::runtime_error("Wrong certificate id " + parent_id.str());
			}

			auto cert = certificate::parse_der(t2.cert.get(st));
			conf.cert_chain.push_front(cert);
			parent_id = cert_control::get_parent_id(cert);
		}
	}

    return true;
  }).then([sp, run_conf, pthis = this->shared_from_this()](){
    if(run_conf->empty()){
      throw std::runtime_error("There is no active network configuration");
    }

    auto result = async_task<>::empty();
    for(auto & conf : *run_conf) {
      pthis->network_services_.push_back(p2p_network_service());

      result = result.then([pthis, conf]() {
        return pthis->network_services_.rbegin()->start(
          conf.scope, conf.port, conf.cert_chain, conf.key);
      });
    }
    return result;
  });
}

vds::async_task<> vds::_p2p_network::prepare_to_stop(const vds::service_provider &sp) {
  if(this->network_services_.empty()){
    return async_task<>::empty();
  }

  return [pthis = this->shared_from_this(), sp](const async_result<> & result) {
    auto runner = new _async_series(result, pthis->network_services_.size());
    for(auto & p : pthis->network_services_){
      runner->add(p.prepare_to_stop(sp));
    }
  };
}

void vds::_p2p_network::stop(const vds::service_provider &sp) {
  for(auto & p : this->network_services_){
    p.stop(sp);
  }
  this->network_services_.clear();
}

std::set<vds::p2p::p2p_node_info> vds::_p2p_network::get_neighbors() const {
  return this->route_->get_neighbors();
}

void vds::_p2p_network::broadcast(
    const service_provider & sp,
    const const_data_buffer & message) const {
  if (this->route_) {
    this->route_->broadcast(sp, message);
  }
}

bool vds::_p2p_network::send(
    const vds::service_provider &sp,
    const vds::guid &device_id,
    const vds::const_data_buffer &message) {
  return this->route_->send(sp, device_id, message);
}

void vds::_p2p_network::process_input_command(
  const service_provider &sp,
  const guid &partner_id,
  const std::shared_ptr<_p2p_crypto_tunnel> &session,
  const const_data_buffer &message_data) {
  binary_deserializer s(message_data);
  uint8_t  command_id;
  s >> command_id;

  switch ((p2p_messages::p2p_message_id)command_id) {
  case p2p_messages::p2p_message_id::common_log_state: {
    p2p_messages::common_log_state message(s);
    sp.get<log_sync_service>()->apply(sp, partner_id, message);

    break;
  }
  case p2p_messages::p2p_message_id::common_block_request: {
    p2p_messages::common_block_request message(s);
    sp.get<log_sync_service>()->apply(sp, partner_id, message);

    break;
  }
  case p2p_messages::p2p_message_id::common_log_record: {
    p2p_messages::common_log_record message(s);
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
    if (sp.get_property<current_run_configuration>(service_provider::property_scope::any_scope)->id() != message.source_node_id()) {
      this->route_->query_replica(sp, message.data_hash(), message.source_node_id());
      sp.get<chunk_replicator>()->apply(sp, partner_id, message);
    }
    break;
  }
    case p2p_messages::p2p_message_id::chunk_offer_replica: {
      p2p_messages::chunk_offer_replica message(s);
      if (sp.get_property<current_run_configuration>(service_provider::property_scope::any_scope)->id() != message.source_node_id()) {
        sp.get<chunk_replicator>()->apply(sp, partner_id, message);
      }
      break;
    }

    case p2p_messages::p2p_message_id::chunk_have_replica: {
      p2p_messages::chunk_have_replica message(s);
      if (sp.get_property<current_run_configuration>(service_provider::property_scope::any_scope)->id() != message.node_id()) {
        sp.get<chunk_replicator>()->apply(sp, partner_id, message);
      }
      break;
    }

  default:
    throw std::runtime_error("Invalid command");
  }
}

void vds::_p2p_network::close_session(
    const vds::service_provider &sp,
    const vds::guid &partner,
    const std::shared_ptr<std::exception> & ex) {
  this->route_->close_session(sp, partner, ex);
}

void vds::_p2p_network::query_replica(
    const service_provider &sp,
    const const_data_buffer & data_hash)
{
	this->route_->query_replica(sp, data_hash);
}

