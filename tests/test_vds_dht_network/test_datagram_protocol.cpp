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
    node1,
    0,
    message).get());

  CHECK_EXPECTED(session2->check_message(
    10,
    message,
    node3,
    node1,
    0));

  CHECK_EXPECTED(session1->proxy_message(
    transport12,
    10,
    node2,
    node3,
    2,
    message).get());
  CHECK_EXPECTED(session2->check_message(
    10,
    message,
    node2,
    node3,
    2));

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
  GET_EXPECTED_GTEST(node1_certificate, vds::_cert_control::create_cert(node1_key));

  GET_EXPECTED_GTEST(node2_key, vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  GET_EXPECTED_GTEST(node2_certificate, vds::_cert_control::create_cert(node2_key));

  GET_EXPECTED_GTEST(node1, node1_certificate.fingerprint(vds::hash::sha256()));
  GET_EXPECTED_GTEST(node2, node2_certificate.fingerprint(vds::hash::sha256()));

  vds::const_data_buffer session_key;
  session_key.resize(32);
  vds::crypto_service::rand_bytes(session_key.data(), session_key.size());

  GET_EXPECTED_GTEST(address, vds::network_address::tcp_ip4("8.8.8.8", 8050));
  auto session1 = std::make_shared<mock_session>(
    sp,
    address,
    node1,
    node2,
    session_key);
  session1->set_mtu(3 * 1024);

  auto session2 = std::make_shared<mock_session>(
    sp,
    address,
    node2,
    node1,
    session_key);
  session1->set_mtu(20 * 1024);

  auto transport12 = std::make_shared<mock_dg_transport>(*session2);
  auto transport21 = std::make_shared<mock_dg_transport>(*session1);

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
      this->shared_from_this(),
      vds::const_data_buffer(data.data(), data.data_size()));
}
