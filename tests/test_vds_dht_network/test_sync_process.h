#ifndef __TEST_VDS_DHT_NETWORK_TEST_SYNC_PROCESS_H_
#define __TEST_VDS_DHT_NETWORK_TEST_SYNC_PROCESS_H_

#include "../../libs/vds_dht_network/private/sync_process.h"
#include "db_model.h"
#include "iudp_transport.h"
#include "network_address.h"

namespace vds {
  class udp_datagram;
}

class test_server;
class mock_transport;
class mock_server;

class message_log_t {
public:
  message_log_t(
    const vds::const_data_buffer& source_node_id,
    const vds::dht::network::imessage_map::message_info_t& message_info)
  : source_node_id_(source_node_id), message_info_(message_info){    
  }

  vds::const_data_buffer source_node_id_;
  vds::dht::network::imessage_map::message_info_t message_info_;
};

enum class message_log_action {
  remove,
  skip
};

class transport_hab : public std::enable_shared_from_this<transport_hab> {
public:
  vds::async_task<> write_async(
    const vds::service_provider& sp,
    const vds::udp_datagram& datagram,
    const vds::const_data_buffer & source_node_id);

  void add(const vds::network_address & address, test_server * server);
  void attach(const std::shared_ptr<test_server> & server1, const std::shared_ptr<test_server>& server2);

  void register_message(
    const vds::service_provider& sp,
    const vds::const_data_buffer& source_node_id,
    const vds::dht::network::imessage_map::message_info_t& message_info);

  void walk_messages(const std::function<message_log_action(const message_log_t &)> & callback);

private:
  std::map<vds::network_address, test_server *> servers_;

  std::mutex log_mutex_;
  std::list<message_log_t> message_log_;
};

class mock_transport : public vds::dht::network::iudp_transport {
public:
  mock_transport(
    mock_server * owner,
    const std::shared_ptr<transport_hab> & hab);

  void start(
    const vds::service_provider& sp,
    const vds::certificate & node_cert,
    const vds::asymmetric_private_key & node_key,
    uint16_t port) override;

  void stop(const vds::service_provider& sp) override;

  vds::async_task<> write_async(const vds::service_provider& sp, const vds::udp_datagram& datagram) override;
  vds::async_task<> try_handshake(const vds::service_provider& sp, const std::string& address) override;

  const vds::const_data_buffer & node_id() const {
    return this->node_id_;
  }
  const vds::network_address & address() const {
    return this->address_;
  }

  void add_session(
    const vds::service_provider& sp,
    const vds::network_address & address,
    const std::shared_ptr<vds::dht::network::dht_session> & session);
  
  const std::shared_ptr<transport_hab> & hab() const {
    return this->hab_;
  }

private:
  mock_server * owner_;
  std::shared_ptr<transport_hab> hab_;
  vds::const_data_buffer node_id_;
  vds::network_address address_;
};

class mock_server : public vds::iservice_factory, protected vds::dht::network::imessage_map {
public:
  mock_server(
    const vds::network_address & address,
    const std::shared_ptr<transport_hab> & hab);

  void register_services(vds::service_registrator &) override;
  void start(const vds::service_provider &) override;
  void stop(const vds::service_provider &) override;
  vds::async_task<> prepare_to_stop(const vds::service_provider &sp) override;

  vds::async_task<> process_datagram(
    const vds::service_provider& sp,
    const vds::udp_datagram& datagram, const vds::const_data_buffer& source_node_id);

  void add_session(
    const vds::service_provider& sp,
    const vds::network_address & address,
    const std::shared_ptr<vds::dht::network::dht_session> & session);
  const vds::const_data_buffer & node_id() const;
  const vds::network_address & address() const;

  void add_sync_entry(
    const vds::service_provider& sp,
    const vds::const_data_buffer& object_id,
    uint32_t object_size);


  vds::async_task<> process_message(const vds::service_provider& scope, const message_info_t& message_info) override;
  void on_new_session(const vds::service_provider& sp, const vds::const_data_buffer& partner_id) override;
private:
  class mock_client : public vds::dht::network::client {
  public:

  };

  vds::db_model db_model_;
  mock_client client_;

  std::shared_ptr<vds::dht::network::iudp_transport> transport_;
  std::map<vds::network_address, std::shared_ptr<vds::dht::network::dht_session>> sessions_;
  vds::network_address address_;

  vds::dht::network::sync_process sync_process_;
};

class test_server {
public:

  test_server(const vds::network_address & address, const std::shared_ptr<transport_hab> & hab);

  void start(const std::shared_ptr<transport_hab> & hab, int index);
  void stop();

  void add_sync_entry(const vds::const_data_buffer& object_id, uint32_t object_size);

  vds::async_task<> process_datagram(
    const vds::service_provider& sp,
    const vds::udp_datagram& datagram,
    const vds::const_data_buffer& source_node_id);

  const vds::const_data_buffer & node_id() const;
  const vds::network_address & address() const;

  void add_session(
    const vds::network_address & address,
    const std::shared_ptr<vds::dht::network::dht_session> & session);

private:

  vds::service_registrator registrator_;
  vds::console_logger logger_;
  vds::mt_service mt_service_;
  vds::task_manager task_manager_;
  mock_server server_;

  vds::service_provider sp_;
};


#endif //__TEST_VDS_DHT_NETWORK_TEST_SYNC_PROCESS_H_
