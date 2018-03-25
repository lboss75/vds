#ifndef __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
#define __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "const_data_buffer.h"

namespace vds {
  class database_transaction;

  namespace dht {
    namespace network {
      class _client;

      class client {
      public:

        const const_data_buffer &current_node_id() const;

        void start(const service_provider & sp, const const_data_buffer &this_node_id);
        void stop(const service_provider & sp);

        template <typename message_type>
        void send(
            const service_provider & sp,
            const const_data_buffer & node_id,
            const message_type & message){
          this->send(sp, node_id, message_type::message_id, message.serialize());
        }

        void save(
            const service_provider & sp,
            database_transaction & t,
            const const_data_buffer & value);

        _client *operator ->() const {
          return this->impl_.get();
        }
      private:
        std::shared_ptr<_client> impl_;

        void send(
            const service_provider & sp,
            const const_data_buffer & node_id,
            const uint8_t message_id,
            const const_data_buffer & message);

        };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
