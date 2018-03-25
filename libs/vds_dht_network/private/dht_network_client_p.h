#ifndef __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_P_H_
#define __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "service_provider.h"
#include "const_data_buffer.h"
#include "dht_session.h"
#include "dht_route.h"
#include "chunk.h"
#include "messages/offer_move_replica.h"
#include "dht_sync_process.h"

namespace vds {
  class database_transaction;

  namespace dht {
    namespace network {
      class _client : public std::enable_shared_from_this<_client> {
      public:
        static constexpr uint16_t MIN_HORCRUX = 512;
        static constexpr uint16_t GENERATE_HORCRUX = 1024;

        _client(
            const service_provider & sp,
            const const_data_buffer & node_id);

        void start(const service_provider & sp);
        void stop(const service_provider & sp);

        void save(
            const service_provider & sp,
            database_transaction & t,
            const const_data_buffer & key,
            const const_data_buffer & value);

        const const_data_buffer &current_node_id() const {
          return this->route_.current_node_id();
        }

        void neighbors(
            const service_provider & sp,
            const const_data_buffer & key,
            std::map<vds::const_data_buffer /*distance*/, std::list<vds::const_data_buffer/*node_id*/>> & result,
            uint16_t max_count) const {
          this->route_.neighbors(sp, key, result, max_count);
        }

        void apply_message(
            const service_provider & sp,
            database_transaction & t,
            const messages::offer_move_replica & message) {
          this->sync_process_.apply_message(sp, t, message);
        }

      private:
        dht_route<std::shared_ptr<dht_session>> route_;
        std::map<uint16_t, chunk_generator<uint16_t>> generators_;
        sync_process sync_process_;

        timer update_timer_;
        bool in_update_timer_;

        void process_update(
            const service_provider & sp,
            database_transaction & t);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
