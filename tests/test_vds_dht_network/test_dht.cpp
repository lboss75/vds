#include "stdafx.h"
#include "test_dht.h"
#include "dht_network.h"
#include "private/cert_control_p.h"

TEST(test_vds_dht_network, test_protocol) {

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

  GET_EXPECTED_GTEST(node3_key, vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  GET_EXPECTED_GTEST(node3_certificate, vds::_cert_control::create_cert(node3_key));

  GET_EXPECTED_GTEST(node4_key, vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  GET_EXPECTED_GTEST(node4_certificate, vds::_cert_control::create_cert(node4_key));


  GET_EXPECTED_GTEST(node1, node1_certificate.fingerprint(vds::hash::sha256()));
  GET_EXPECTED_GTEST(node2, node2_certificate.fingerprint(vds::hash::sha256()));
  GET_EXPECTED_GTEST(node3, node3_certificate.fingerprint(vds::hash::sha256()));
  GET_EXPECTED_GTEST(node4, node4_certificate.fingerprint(vds::hash::sha256()));

  vds::const_data_buffer session_key;
  session_key.resize(32);
  vds::crypto_service::rand_bytes(session_key.data(), session_key.size());

  GET_EXPECTED_GTEST(address, vds::network_address::tcp_ip4("8.8.8.8", 8050));
  auto session1 = std::make_shared<mock_unreliable_session>(
    sp,
    address,
    node1,
    node2,
    session_key);
  //session1->set_mtu(1024);

  auto session2 = std::make_shared<mock_unreliable_session>(
    sp,
    address,
    node2,
    node1,
    session_key);
  //session1->set_mtu(20 * 1024);

  auto transport12 = std::make_shared<mock_unreliable_transport>(*session2);
  auto transport21 = std::make_shared<mock_unreliable_transport>(*session1);
  transport12->set_partner(transport21);
  transport21->set_partner(transport12);

  std::vector<std::tuple<
    uint8_t /*message_type_*/,
    vds::const_data_buffer /*message_*/,
    vds::const_data_buffer /*target_node_*/,
    vds::const_data_buffer /*source_node_*/,
    uint16_t /*hops_*/
    >> messages;

  for (int i = 0; i < 10000; ++i)
  {
    const size_t size = std::rand() & 0xFFFFFFFF;

    vds::const_data_buffer message;
    message.resize(size);
    for (size_t b = 0; b < size; ++b) {
      message[b] = static_cast<uint8_t>(std::rand());
    }

    uint8_t message_type = (uint8_t)std::rand() % 32;
    vds::const_data_buffer target_node;
    vds::const_data_buffer source_node;
    uint16_t hops;


    if (std::rand() < 10000) {
      target_node.resize(32);
      vds::crypto_service::rand_bytes(target_node.data(), target_node.size());
      source_node.resize(32);
      vds::crypto_service::rand_bytes(source_node.data(), source_node.size());
      hops = (uint8_t)std::rand() % 64;
      CHECK_EXPECTED_GTEST(session1->proxy_message(transport12, message_type, target_node, source_node, hops, message).get());
    }
    else if (std::rand() < 10000) {
      target_node.resize(32);
      vds::crypto_service::rand_bytes(target_node.data(), target_node.size());
      source_node = node1;
      hops = 0;

      CHECK_EXPECTED_GTEST(session1->send_message(transport12, message_type, target_node, message).get());
    }
    else {
      target_node = node2;
      source_node = node1;
      hops = 0;
      CHECK_EXPECTED_GTEST(session1->send_message(transport12, message_type, node2, message).get());
    }

    messages.push_back(std::make_tuple(message_type, message, target_node, source_node, hops));
  }

  for (int i = 0; i < messages.size(); ++i)
  {
    GET_EXPECTED_GTEST(
      isFinish,
      session2->check_message(
        transport21,
        std::get<0>(messages[i]),
        std::get<1>(messages[i]),
        std::get<2>(messages[i]),
        std::get<3>(messages[i]),
        std::get<4>(messages[i])));
    if (!isFinish) {
      GTEST_ASSERT_LT(messages.size() - 10, i);
    }
  }

  CHECK_EXPECTED_GTEST(registrator.shutdown());
}

vds::async_task<vds::expected<void>> mock_unreliable_transport::write_async(const vds::udp_datagram & data)
{
  if ((3 & std::rand()) == 0)
  {
    return vds::async_task<vds::expected<void>>(vds::expected<void>());
  }
  else
  {
    return this->s_.process_datagram(
      this->partner_.lock(),
      vds::const_data_buffer(data.data(), data.data_size()));
  }
}
