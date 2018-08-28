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

class transport_hab : public std::enable_shared_from_this<transport_hab> {
public:
  vds::async_task<> write_async(
    const vds::service_provider& sp,
    const vds::udp_datagram& datagram,
    const vds::const_data_buffer & source_node_id);

  vds::async_task<> try_handshake(
    const vds::service_provider& sp,
    const std::string& address,
    const vds::const_data_buffer & source_node_id);

  void add(const vds::network_address & address, test_server * server);

private:
  std::map<vds::network_address, test_server *> servers_;
};

class test_server {
public:

  test_server(const std::shared_ptr<transport_hab> & hab);

  void start(const std::shared_ptr<transport_hab> & hab, int index);
  void stop();

  void add_sync_entry(const vds::const_data_buffer& object_id, uint32_t object_size);

  vds::async_task<> process_datagram(const vds::service_provider& sp, const vds::udp_datagram& datagram, const vds::const_data_buffer& source_node_id);
  vds::async_task<> try_handshake(const vds::service_provider& sp, const vds::const_data_buffer& source_node_id);


private:

  class mock_transport : public vds::dht::network::iudp_transport {
  public:
    mock_transport(
      const std::shared_ptr<transport_hab> & hab);

    void start(
      const vds::service_provider& sp,
      const vds::certificate & node_cert,
      const vds::asymmetric_private_key & node_key,
      uint16_t port) override;

    void stop(const vds::service_provider& sp) override;

    vds::async_task<> write_async(const vds::service_provider& sp, const vds::udp_datagram& datagram) override;
    vds::async_task<> try_handshake(const vds::service_provider& sp, const std::string& address) override;

  private:
    std::shared_ptr<transport_hab> hab_;
    vds::const_data_buffer node_id_;
  };

  class mock_server : public vds::iservice_factory {
  public:
    mock_server(const std::shared_ptr<vds::dht::network::iudp_transport> & transport)
    : transport_(transport) {
      
    }

    void register_services(vds::service_registrator &) override;
    void start(const vds::service_provider &) override;
    void stop(const vds::service_provider &) override;
    vds::async_task<> prepare_to_stop(const vds::service_provider &sp) override;

    vds::async_task<> process_datagram(const vds::service_provider& sp, const vds::udp_datagram& datagram, const vds::const_data_buffer& source_node_id);
    vds::async_task<> try_handshake(const vds::service_provider& sp, const vds::const_data_buffer& source_node_id);
  private:
    class mock_client : public vds::dht::network::client {
    public:

    };

    vds::db_model db_model_;
    mock_client client_;

    std::shared_ptr<vds::dht::network::iudp_transport> transport_;
    std::map<vds::network_address, std::shared_ptr<vds::dht::network::dht_session>> sessions_;
  };

  vds::service_registrator registrator_;
  vds::console_logger logger_;
  vds::mt_service mt_service_;
  mock_server server_;


  vds::service_provider sp_;

  vds::dht::network::sync_process sync_process_;


};


#endif //__TEST_VDS_DHT_NETWORK_TEST_SYNC_PROCESS_H_
