#include "stdafx.h"
#include "test_datagram_protocol.h"
#include "cert_control.h"
#include "private/cert_control_p.h"
#include "test_config.h"

static void send_message_check(
  const vds::service_provider & sp,
  const vds::const_data_buffer & node1,
  const vds::const_data_buffer & node2,
  const std::shared_ptr<mock_session> & session1,
  const std::shared_ptr<mock_session> & session2,
  const std::shared_ptr<mock_transport> & transport12,
  const std::shared_ptr<mock_transport> & transport21,
  size_t size) {

  vds::const_data_buffer message;
  message.resize(size);
  for(size_t i = 0; i < size; ++i){
    message[i] = std::rand();
  }

  session1->send_message(sp, transport12, 10, node2, message);
  session1->check_message(10, message);

  session2->send_message(sp, transport21, 10, node1, message);
  session2->check_message(10, message);
}

TEST(test_vds_dht_network, test_data_exchange) {
  vds::service_registrator registrator;

  vds::console_logger logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::mt_service mt_service;

  registrator.add(logger);
  registrator.add(mt_service);

  auto sp = registrator.build("test_async_stream");
  registrator.start(sp);

  vds::mt_service::enable_async(sp);

  auto cert = vds::cert_control::get_storage_certificate();
  auto key = vds::cert_control::get_common_storage_private_key();

  const auto node1_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  const auto node1_certificate = vds::_cert_control::create_cert(
      "Node1",
      node1_key,
      cert,
      key);

  const auto node2_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  const auto node2_certificate = vds::_cert_control::create_cert(
      "Node2",
      node2_key,
      cert,
      key);

  vds::const_data_buffer node1 = node1_certificate.fingerprint(vds::hash::sha256());
  vds::const_data_buffer node2 = node2_certificate.fingerprint(vds::hash::sha256());

  vds::const_data_buffer session_key;
  session_key.resize(32);
  vds::crypto_service::rand_bytes(session_key.data(), session_key.size());

  auto session1 = std::make_shared<mock_session>(
    vds::network_address(AF_INET, 8050),
    node1,
    node2,
    session_key);
  session1->set_mtu(5 * 1024);

  auto session2 = std::make_shared<mock_session>(
    vds::network_address(AF_INET, 8051),
    node2,
    node1,
    session_key);

  auto transport12 = std::make_shared<mock_transport>(*session2);
  auto transport21 = std::make_shared<mock_transport>(*session1);

  send_message_check(sp, node1, node2, session1, session2, transport12, transport21, 10);
  send_message_check(sp, node1, node2, session1, session2, transport12, transport21, 10 * 1024);
  send_message_check(sp, node1, node2, session1, session2, transport12, transport21, 10 * 1024 * 1024);

  registrator.shutdown(sp);
}

vds::async_task<> mock_transport::write_async(
    const vds::service_provider &sp,
    const vds::udp_datagram &data) {
  return this->s_.process_datagram(
      sp,
      this->shared_from_this(),
      vds::const_data_buffer(data.data(), data.data_size()));
}
