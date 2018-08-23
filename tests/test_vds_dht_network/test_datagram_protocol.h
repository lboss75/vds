#ifndef __TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_
#define __TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_

#include "../private/dht_datagram_protocol.h"

class mock_transport {
  
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

};

#endif //__TEST_VDS_DHT_NETWORK_TEST_DATAGRAM_PROTOCOL_H_
