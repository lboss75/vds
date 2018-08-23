#include "stdafx.h"
#include "test_datagram_protocol.h"

static void send_message_check(
  const vds::service_provider & sp,
  const vds::const_data_buffer & node1,
  const vds::const_data_buffer & node2,
  const std::shared_ptr<mock_session> & session1,
  const std::shared_ptr<mock_session> & session2,
  const std::shared_ptr<mock_transport> & transport12,
  const std::shared_ptr<mock_transport> & transport21,
  size_t size) {

  session1->send_message(sp, transport12, 10, node1, message);
  transport12.check(10, message);

  session2->send_message(sp, transport21, 10, node2, message);
  transport21.check(10, message);
}

TEST(test_vds_dht_network, test_data_exchange) {

  vds::const_data_buffer node1;
  vds::const_data_buffer node2;


  auto session1 = std::make_shared<mock_session>(
    vds::network_address(AF_INET, 8050),
    node1,
    node2,
    0);

  auto session2 = std::make_shared<mock_session>(
    vds::network_address(AF_INET, 8051),
    node2,
    node1,
    0);

  auto transport12 = std::make_shared<mock_transport>();
  auto transport21 = std::make_shared<mock_transport>();

  send_message_check(sp, node1, node2, session1, session2, transport12, transport21, 10);
  send_message_check(sp, node1, node2, session1, session2, transport12, transport21, 10 * 1024);
  send_message_check(sp, node1, node2, session1, session2, transport12, transport21, 10 * 1024 * 1024);
}