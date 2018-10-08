#ifndef __TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_
#define __TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_

#include "../private/dht_datagram_protocol.h"

class mock_session;

class mock_transport : public std::enable_shared_from_this<mock_transport> {
public:
  mock_transport(mock_session & s)
  : s_(s){
  }
  std::future<void> write_async(
      
      const vds::udp_datagram & data);

private:
  mock_session & s_;
};

class mock_session : public vds::dht::network::dht_datagram_protocol<mock_session, mock_transport> {
  typedef vds::dht::network::dht_datagram_protocol<mock_session, mock_transport> base_class;
public:
  mock_session(
    const vds::service_provider * sp,
    const vds::network_address& address,
    const vds::const_data_buffer& this_node_id,
    const vds::const_data_buffer& partner_node_id,
    const vds::const_data_buffer& session_key)
    : base_class(sp, address, this_node_id, partner_node_id, session_key) {
  }

  std::future<void> process_message(
      
      const std::shared_ptr<mock_transport>& transport,
      uint8_t message_type,
      const vds::const_data_buffer & target_node,
      const vds::const_data_buffer & source_node,
      uint16_t hops,
      const vds::const_data_buffer& message) {

    this->message_type_ = message_type;
    this->message_ = message;
    this->target_node_ = target_node;
    this->source_node_ = source_node;
    this->hops_ = hops;

    co_return;
  }


  void check_message(uint8_t message_type, const vds::const_data_buffer & message){
    for(int i = 0; i < 10; ++i) {
      if(!this->message_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      else {
        break;
      }
    }
    if(message_type != this->message_type_) {
      throw std::runtime_error("Invalid message type");
    }
    if (message != this->message_) {
      throw std::runtime_error("Invalid message");
    }

    this->message_.clear();
  }

  void check_message(
    uint8_t message_type,
    const vds::const_data_buffer & message,
    const vds::const_data_buffer & target_node,
    const vds::const_data_buffer & source_node,
    uint16_t hops) {
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
      throw std::runtime_error("Invalid message type");
    }
    if (message != this->message_) {
      throw std::runtime_error("Invalid message");
    }
    if (target_node != this->target_node_) {
      throw std::runtime_error("Invalid target_node");
    }
    if (source_node != this->source_node_) {
      throw std::runtime_error("Invalid source_node");
    }
    if (hops != this->hops_) {
      throw std::runtime_error("Invalid hops");
    }

    this->message_.clear();
  }

  private:
    uint8_t message_type_;
    vds::const_data_buffer message_;
    vds::const_data_buffer target_node_;
    vds::const_data_buffer source_node_;
    uint16_t hops_;
};

#endif //__TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_
