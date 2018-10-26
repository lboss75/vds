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

namespace vds {
  class database_transaction;
  class database_read_transaction;
  class certificate;
  class asymmetric_private_key;


  namespace dht {
    namespace network {
      class iudp_transport;
      enum class message_type_t;
      class _client;

      class client {
      public:
        void start(
          const service_provider * sp,
          const std::shared_ptr<certificate> & node_cert,
          const std::shared_ptr<asymmetric_private_key> & node_key,
          const std::shared_ptr<iudp_transport> & udp_transport);

        void stop();

        struct chunk_info {
          const_data_buffer id;
          const_data_buffer key;
          std::vector<const_data_buffer> object_ids;
        };

        struct block_info_t {
          std::map<const_data_buffer, std::list<uint16_t>> replicas;
        };

        async_task<chunk_info> save(
          
          database_transaction& t,
          const const_data_buffer& value);

        async_task<const_data_buffer> restore(          
          const chunk_info& block_id);

        async_task<block_info_t> prepare_restore(
          database_read_transaction & t,
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
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
