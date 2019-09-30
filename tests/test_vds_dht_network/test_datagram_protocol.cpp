#include "stdafx.h"
#include "test_datagram_protocol.h"
#include "keys_control.h"
#include "private/cert_control_p.h"

static vds::expected<void> send_message_check(
  
  const vds::const_data_buffer & node1,
  const vds::const_data_buffer & node2,
  const std::shared_ptr<mock_session> & session1,
  const std::shared_ptr<mock_session> & session2,
  const std::shared_ptr<mock_dg_transport> & transport12,
  const std::shared_ptr<mock_dg_transport> & transport21,
  size_t size) {

  vds::const_data_buffer message;
  message.resize(size);
  for(size_t i = 0; i < size; ++i){
    message[i] = static_cast<uint8_t>(std::rand());
  }

  CHECK_EXPECTED(session1->send_message(transport12, 10, node2, message).get());
  CHECK_EXPECTED(session2->check_message(10, message));

  CHECK_EXPECTED(session2->send_message(transport21, 10, node1, message).get());
  CHECK_EXPECTED(session1->check_message(10, message));

  return vds::expected<void>();
}

static vds::expected<void> proxy_message_check(
  
  const vds::const_data_buffer & node1,
  const vds::const_data_buffer & node2,
  const std::shared_ptr<mock_session> & session1,
  const std::shared_ptr<mock_session> & session2,
  const std::shared_ptr<mock_dg_transport> & transport12,
  const std::shared_ptr<mock_dg_transport> & transport21,
  size_t size) {

  vds::const_data_buffer message;
  message.resize(size);
  for (size_t i = 0; i < size; ++i) {
    message[i] = std::rand();
  }

  vds::const_data_buffer node3;
  node3.resize(32);

  vds::crypto_service::rand_bytes(node3.data(), node3.size());
  CHECK_EXPECTED(session1->proxy_message(
    transport12,
    10,
    node3,
    std::vector<vds::const_data_buffer>({ node3 }),
    message).get());

  CHECK_EXPECTED(session2->check_message(
    10,
    message,
    node3,
    std::vector<vds::const_data_buffer>({ node1, node2 })));

  CHECK_EXPECTED(session1->proxy_message(
    transport12,
    10,
    node2,
    std::vector<vds::const_data_buffer>({ node3, node1 }),
    message).get());
  CHECK_EXPECTED(session2->check_message(
    10,
    message,
    node2,
    std::vector<vds::const_data_buffer>({ node1, node3, node1 })));

  return vds::expected<void>();
}


TEST(test_vds_dht_network, test_data_exchange) {

#ifdef _WIN32
  //Initialize Winsock
  WSADATA wsaData;
  if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
    auto error = WSAGetLastError();
    GTEST_FATAL_FAILURE_(std::system_error(error, std::system_category(), "Initiates Winsock").what());
  }
#endif

  vds::service_registrator registrator;

  vds::console_logger logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::mt_service mt_service;

  registrator.add(logger);
  registrator.add(mt_service);

  GET_EXPECTED_GTEST(sp, registrator.build());
  CHECK_EXPECTED_GTEST(registrator.start());

  GET_EXPECTED_GTEST(node1_key, vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  GET_EXPECTED_GTEST(node1_certificate, vds::asymmetric_public_key::create(node1_key));

  GET_EXPECTED_GTEST(node2_key, vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  GET_EXPECTED_GTEST(node2_certificate, vds::asymmetric_public_key::create(node2_key));

  GET_EXPECTED_GTEST(node1, node1_certificate.fingerprint());
  GET_EXPECTED_GTEST(node2, node2_certificate.fingerprint());

  vds::const_data_buffer session_key;
  session_key.resize(32);
  vds::crypto_service::rand_bytes(session_key.data(), session_key.size());

  GET_EXPECTED_GTEST(address, vds::network_address::tcp_ip4("8.8.8.8", 8050));
  auto session1 = std::make_shared<mock_session>(
    sp,
    address,
    node1,
    std::move(node2_certificate),
    node2,
    session_key);
  session1->set_mtu(3 * 1024);

  auto session2 = std::make_shared<mock_session>(
    sp,
    address,
    node2,
    std::move(node1_certificate),
    node1,
    session_key);
  session1->set_mtu(20 * 1024);

  auto transport12 = std::make_shared<mock_dg_transport>(*session2);
  auto transport21 = std::make_shared<mock_dg_transport>(*session1);
  transport12->set_partner(transport21);
  transport21->set_partner(transport12);

  CHECK_EXPECTED_GTEST(send_message_check(node1, node2, session1, session2, transport12, transport21, 10));
  CHECK_EXPECTED_GTEST(send_message_check(node1, node2, session1, session2, transport12, transport21, 10 * 1024));
  CHECK_EXPECTED_GTEST(send_message_check(node1, node2, session1, session2, transport12, transport21, 0xFFFF));
  CHECK_EXPECTED_GTEST(send_message_check(node1, node2, session1, session2, transport12, transport21, 10 * 0xFFFF));


  CHECK_EXPECTED_GTEST(proxy_message_check(node1, node2, session1, session2, transport12, transport21, 10));
  CHECK_EXPECTED_GTEST(proxy_message_check(node1, node2, session1, session2, transport12, transport21, 10 * 1024));
  CHECK_EXPECTED_GTEST(proxy_message_check(node1, node2, session1, session2, transport12, transport21, 0xFFFF));
  CHECK_EXPECTED_GTEST(proxy_message_check(node1, node2, session1, session2, transport12, transport21, 10 * 0xFFFF));

  CHECK_EXPECTED_GTEST(registrator.shutdown());
}

vds::async_task<vds::expected<void>> mock_dg_transport::write_async(
    
    const vds::udp_datagram &data) {
  return this->s_.process_datagram(
      this->partner_.lock(),
      vds::const_data_buffer(data.data(), data.data_size()));
}

vds::async_task<vds::expected<void>> mock_dg_transport::start(
  const vds::service_provider* sp,
  const std::shared_ptr<vds::asymmetric_public_key>& node_public_key,
  const std::shared_ptr<vds::asymmetric_private_key>& node_key,
  uint16_t port,
  bool dev_network)
{
  throw vds::vds_exceptions::invalid_operation();
}

void mock_dg_transport::stop()
{
}

vds::async_task<vds::expected<void>> mock_dg_transport::try_handshake(const std::string& address)
{
  throw vds::vds_exceptions::invalid_operation();
}

vds::expected<void> mock_dg_transport::broadcast_handshake()
{
  throw vds::vds_exceptions::invalid_operation();
}

vds::async_task<vds::expected<void>> mock_dg_transport::on_timer()
{
  throw vds::vds_exceptions::invalid_operation();
}
