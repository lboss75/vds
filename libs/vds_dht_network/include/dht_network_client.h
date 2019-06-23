#ifndef __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
#define __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "const_data_buffer.h"
#include "route_statistic.h"
#include "session_statistic.h"
#include "stream.h"

namespace vds {
  class database_transaction;
  class database_read_transaction;
  class asymmetric_public_key;
  class asymmetric_private_key;

  namespace transactions {
    class transaction_block_builder;
  }

  namespace dht {
    namespace network {
      class iudp_transport;
      enum class message_type_t;
      class _client;

      class client {
      public:

        client();

        expected<void> load_keys(
          database_transaction & t);

        expected<void> start(
          const service_provider * sp,
          const std::shared_ptr<iudp_transport> & udp_transport,
          uint16_t port,
          bool dev_network);

        expected<void> stop();

        struct chunk_info {
          const_data_buffer id;
          const_data_buffer key;
          std::vector<const_data_buffer> object_ids;
        };

        struct block_info_t {
          std::map<const_data_buffer, std::list<uint16_t>> replicas;
        };

        expected<chunk_info> save(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& value);

        expected<const_data_buffer> save(
          const service_provider * sp,
          transactions::transaction_block_builder & block,
          database_transaction& t);

        expected<std::shared_ptr<stream_output_async<uint8_t>>> start_save(
          const service_provider * sp) const;

        async_task<expected<chunk_info>> finish_save(
          const service_provider * sp,
          std::shared_ptr<stream_output_async<uint8_t>> stream);

        async_task<expected<const_data_buffer>> restore(          
          const chunk_info& block_id);

        expected<block_info_t> prepare_restore(
          database_read_transaction & t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const chunk_info& block_id);

        const const_data_buffer& current_node_id() const;

        void get_route_statistics(route_statistic& result);
        void get_session_statistics(session_statistic& session_statistic);

        _client* operator ->() const {
          return this->impl_.get();
        }

        void update_wellknown_connection_enabled(bool value);

      private:
        std::shared_ptr<_client> impl_;
        std::shared_ptr<asymmetric_public_key> node_public_key_;
        std::shared_ptr<asymmetric_private_key> node_key_;
        bool is_new_node_;
        std::unique_ptr<async_task<expected<void>>> udp_transport_task_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
