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
  GET_EXPECTED_GTEST(node1_certificate, vds::asymmetric_public_key::create(node1_key));

  GET_EXPECTED_GTEST(node2_key, vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  GET_EXPECTED_GTEST(node2_certificate, vds::asymmetric_public_key::create(node2_key));

  GET_EXPECTED_GTEST(node3_key, vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  GET_EXPECTED_GTEST(node3_certificate, vds::asymmetric_public_key::create(node3_key));

  GET_EXPECTED_GTEST(node4_key, vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096()));
  GET_EXPECTED_GTEST(node4_certificate, vds::asymmetric_public_key::create(node4_key));


  GET_EXPECTED_GTEST(node1, node1_certificate.fingerprint());
  GET_EXPECTED_GTEST(node2, node2_certificate.fingerprint());
  GET_EXPECTED_GTEST(node3, node3_certificate.fingerprint());
  GET_EXPECTED_GTEST(node4, node4_certificate.fingerprint());

  vds::const_data_buffer session_key;
  session_key.resize(32);
  vds::crypto_service::rand_bytes(session_key.data(), session_key.size());

  GET_EXPECTED_GTEST(address, vds::network_address::tcp_ip4("8.8.8.8", 8050));
  auto session1 = std::make_shared<mock_unreliable_session>(
    sp,
    address,
    node1,
    std::move(node2_certificate),
    node2,
    session_key);
  //session1->set_mtu(1024);

  auto session2 = std::make_shared<mock_unreliable_session>(
    sp,
    address,
    node2,
    std::move(node1_certificate),
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
    std::vector<vds::const_data_buffer> /*hops_*/
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
    std::vector<vds::const_data_buffer> hops;

    if (std::rand() < 10000) {
      target_node.resize(32);
      vds::crypto_service::rand_bytes(target_node.data(), target_node.size());
      source_node.resize(32);
      vds::crypto_service::rand_bytes(source_node.data(), source_node.size());

      hops.push_back(node1);
      hops.push_back(source_node);

      CHECK_EXPECTED_GTEST(session1->proxy_message(
        transport12,
        message_type,
        target_node,
        std::vector<vds::const_data_buffer>({ source_node }),
        message).get());
    }
    else if (std::rand() < 10000) {
      target_node.resize(32);
      vds::crypto_service::rand_bytes(target_node.data(), target_node.size());
      source_node = node1;
      hops.push_back(node1);

      CHECK_EXPECTED_GTEST(session1->send_message(transport12, message_type, target_node, message).get());
    }
    else {
      target_node = node2;
      source_node = node1;
      hops.push_back(node1);
      CHECK_EXPECTED_GTEST(session1->send_message(transport12, message_type, node2, message).get());
    }

    messages.push_back(std::make_tuple(message_type, message, target_node, hops));
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
        std::get<3>(messages[i])));
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

vds::async_task<vds::expected<void>> mock_unreliable_transport::start(const vds::service_provider* sp, const std::shared_ptr<vds::asymmetric_public_key>& node_public_key, const std::shared_ptr<vds::asymmetric_private_key>& node_key, uint16_t port, bool dev_network)
{
  throw vds::vds_exceptions::invalid_operation();
}

void mock_unreliable_transport::stop()
{
  throw vds::vds_exceptions::invalid_operation();
}

vds::async_task<vds::expected<void>> mock_unreliable_transport::try_handshake(const std::string& address)
{
  throw vds::vds_exceptions::invalid_operation();
}

vds::expected<void> mock_unreliable_transport::broadcast_handshake()
{
  throw vds::vds_exceptions::invalid_operation();
}

vds::async_task<vds::expected<void>> mock_unreliable_transport::on_timer()
{
  throw vds::vds_exceptions::invalid_operation();
}
