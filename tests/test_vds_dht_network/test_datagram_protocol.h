#ifndef __TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_
#define __TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_

#include "../private/dht_datagram_protocol.h"

class mock_session;

class mock_transport : public std::enable_shared_from_this<mock_transport> {
public:
  mock_transport(mock_session & s)
  : s_(s){
  }
  vds::async_task<> write_async(
      const vds::service_provider & sp,
      const vds::udp_datagram & data);

private:
  mock_session & s_;
};

class mock_session : public vds::dht::network::dht_datagram_protocol<mock_session, mock_transport> {
  typedef vds::dht::network::dht_datagram_protocol<mock_session, mock_transport> base_class;
public:
  mock_session(
    const vds::network_address& address,
    const vds::const_data_buffer& this_node_id,
    const vds::const_data_buffer& partner_node_id,
    uint32_t session_id)
    : base_class(address, this_node_id, partner_node_id, session_id) {
  }

  vds::async_task<> process_message(
      const vds::service_provider& sp,
      const std::shared_ptr<mock_transport>& transport,
      uint8_t message_type,
      const vds::const_data_buffer & target_node,
      const vds::const_data_buffer & source_node,
      uint32_t source_index,
      uint16_t hops,
      const vds::const_data_buffer& message) {

    this->message_type_ = message_type;
    this->message_ = message;
    this->target_node_ = target_node;
    this->source_node_ = source_node;
    this->source_index_ = source_index;
    this->hops_ = hops;

    return vds::async_task<>::empty();
  }


  void check_message(uint8_t message_type, const vds::const_data_buffer & message){
    GTEST_ASSERT_EQ(message_type, this->message_type_);
    GTEST_ASSERT_EQ(message, this->message_);
  }

  private:
    uint8_t message_type_;
    vds::const_data_buffer message_;
    vds::const_data_buffer target_node_;
    vds::const_data_buffer source_node_;
    uint32_t source_index_;
    uint16_t hops_;
};

#endif //__TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_
