#ifndef __TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_
#define __TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_

#include "../private/dht_datagram_protocol.h"
#include "iudp_transport.h"

class mock_session;

class mock_dg_transport : public vds::dht::network::iudp_transport {
public:
  mock_dg_transport(mock_session & s)
  : s_(s){
  }
  vds::async_task<vds::expected<void>> write_async(
      const vds::udp_datagram & data) override;

  void set_partner(std::shared_ptr<mock_dg_transport> partner)
  {
    this->partner_ = partner;
  }

private:
  mock_session & s_;
  std::weak_ptr<mock_dg_transport> partner_;

  // Inherited via iudp_transport
  vds::async_task<vds::expected<void>> start(
    const vds::service_provider* sp,
    const std::shared_ptr<vds::asymmetric_public_key>& node_public_key,
    const std::shared_ptr<vds::asymmetric_private_key>& node_key,
    uint16_t port,
    bool dev_network) override;

  void stop() override;
  vds::async_task<vds::expected<void>> try_handshake(const std::string& address) override;
  vds::expected<void> broadcast_handshake() override;
  vds::async_task<vds::expected<void>> on_timer() override;
};

class mock_session : public vds::dht::network::dht_datagram_protocol {
  typedef vds::dht::network::dht_datagram_protocol base_class;
public:
  mock_session(
    const vds::service_provider * sp,
    const vds::network_address& address,
    const vds::const_data_buffer& this_node_id,
    vds::asymmetric_public_key partner_node_key,
    const vds::const_data_buffer& partner_node_id,
    const vds::const_data_buffer& session_key)
    : base_class(sp, address, this_node_id, std::move(partner_node_key), partner_node_id, session_key) {
  }

  vds::async_task<vds::expected<bool>> process_message(
      const std::shared_ptr<vds::dht::network::iudp_transport>& transport,
      uint8_t message_type,
      const vds::const_data_buffer & target_node,
      const std::vector<vds::const_data_buffer>& hops,
      const vds::const_data_buffer& message) override {

    this->message_type_ = message_type;
    this->message_ = message;
    this->target_node_ = target_node;
    this->hops_ = hops;

    co_return true;
  }


  vds::expected<void> check_message(uint8_t message_type, const vds::const_data_buffer & message){
    for(int i = 0; i < 10; ++i) {
      if(!this->message_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      else {
        break;
      }
    }
    if(message_type != this->message_type_) {
      return vds::make_unexpected<std::runtime_error>("Invalid message type");
    }
    if (message != this->message_) {
      return vds::make_unexpected<std::runtime_error>("Invalid message");
    }

    this->message_.clear();
    return vds::expected<void>();
  }

  vds::expected<void> check_message(
    uint8_t message_type,
    const vds::const_data_buffer & message,
    const vds::const_data_buffer & target_node,
    const std::vector<vds::const_data_buffer>& hops) {
    int i;
    for (i = 0; i < 10; ++i) {
      if (!this->message_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      else {
        break;
      }
    }
    if (message_type != this->message_type_) {
      return vds::make_unexpected<std::runtime_error>("Invalid message type");
    }
    if (message != this->message_) {
      return vds::make_unexpected<std::runtime_error>("Invalid message");
    }
    if (target_node != this->target_node_) {
      return vds::make_unexpected<std::runtime_error>("Invalid target_node");
    }

    if (hops.size() != this->hops_.size()) {
      return vds::make_unexpected<std::runtime_error>("Invalid hops");
    }
    for (int i = 0; i < hops.size(); ++i) {
      if (this->hops_[i] != hops_[i]) {
        return vds::make_unexpected<std::runtime_error>("Invalid hop node");
      }
    }

    this->message_.clear();
    return vds::expected<void>();
  }

  private:
    uint8_t message_type_;
    vds::const_data_buffer message_;
    vds::const_data_buffer target_node_;
    std::vector<vds::const_data_buffer> hops_;
};

#endif //__TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_
