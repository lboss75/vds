//
// Created by vadim on 07.06.18.
//

#ifndef __TEST_VDS_DHT_NETWORK_VDS_TEST_DHT_H_
#define __TEST_VDS_DHT_NETWORK_VDS_TEST_DHT_H_

#include "udp_socket.h"
#include "../private/dht_datagram_protocol.h"
#include "iudp_transport.h"

class mock_unreliable_session;

class mock_unreliable_transport : public vds::dht::network::iudp_transport {
public:
  mock_unreliable_transport(mock_unreliable_session & s)
    : s_(s) {
  }
  vds::async_task<vds::expected<void>> write_async(
    const vds::udp_datagram & data);

  void set_partner(std::shared_ptr<mock_unreliable_transport> partner)
  {
    this->partner_ = partner;
  }

private:
  mock_unreliable_session & s_;
  std::weak_ptr<mock_unreliable_transport> partner_;

  // Inherited via iudp_transport
  virtual vds::async_task<vds::expected<void>> start(const vds::service_provider* sp, const std::shared_ptr<vds::asymmetric_public_key>& node_public_key, const std::shared_ptr<vds::asymmetric_private_key>& node_key, uint16_t port, bool dev_network) override;
  virtual void stop() override;
  virtual vds::async_task<vds::expected<void>> try_handshake(const std::string& address) override;
  virtual vds::expected<void> broadcast_handshake() override;
  virtual vds::async_task<vds::expected<void>> on_timer() override;
};


class mock_unreliable_session : public vds::dht::network::dht_datagram_protocol {
  typedef vds::dht::network::dht_datagram_protocol base_class;
public:
  mock_unreliable_session(
    const vds::service_provider * sp,
    const vds::network_address& address,
    const vds::const_data_buffer& this_node_id,
    vds::asymmetric_public_key partner_node_key,
    const vds::const_data_buffer& partner_node_id,
    const vds::const_data_buffer& session_key)
    : base_class(sp, address, this_node_id, std::move(partner_node_key),  partner_node_id, session_key) {
  }

  vds::async_task<vds::expected<bool>> process_message(
    const std::shared_ptr<vds::dht::network::iudp_transport>& transport,
    uint8_t message_type,
    const vds::const_data_buffer & target_node,
    const std::vector<vds::const_data_buffer>& hops,
    const vds::const_data_buffer& message) override {

    this->messages_.push_back(std::make_tuple(
      message_type,
      message,
      target_node,
      hops));

    co_return true;
  }


  vds::expected<bool> check_message(
    const std::shared_ptr<mock_unreliable_transport> & s,
    uint8_t message_type,
    const vds::const_data_buffer & message,
    const vds::const_data_buffer & target_node,
    const std::vector<vds::const_data_buffer> & hops) {
    for (int i = 0; i < 10; ++i) {
      if (this->messages_.empty()) {
        CHECK_EXPECTED(this->send_acknowledgment(std::static_pointer_cast<vds::dht::network::iudp_transport>(s)).get());

        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      else {
        break;
      }
    }

    if (this->messages_.empty()) {
      return vds::expected<bool>(false);
    }
    const auto & p = this->messages_.front();
    if (message_type != std::get<0>(p)) {
      return vds::make_unexpected<std::runtime_error>("Invalid message type");
    }
    if (message != std::get<1>(p)) {
      return vds::make_unexpected<std::runtime_error>("Invalid message");
    }
    if (target_node != std::get<2>(p)) {
      return vds::make_unexpected<std::runtime_error>("Invalid target_node");
    }
    if (hops.size() != std::get<3>(p).size()) {
      return vds::make_unexpected<std::runtime_error>("Invalid hops");
    }
    if (!std::equal(hops.begin(), hops.end(), std::get<3>(p).begin())) {
      return vds::make_unexpected<std::runtime_error>("Invalid hops");
    }

    this->messages_.pop_front();

    return vds::expected<bool>(true);
  }

private:
  typedef std::tuple<
    uint8_t /*message_type_*/,
    vds::const_data_buffer /*message_*/,
    vds::const_data_buffer /*target_node_*/,
    std::vector<vds::const_data_buffer> /*hops_*/
  > message_type;

  std::list<message_type> messages_;
};



#endif //__TEST_VDS_DHT_NETWORK_VDS_TEST_DHT_H_
