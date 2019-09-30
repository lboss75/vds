#ifndef __VDS_DHT_NETWORK_IMESSAGE_MAP_H_
#define __VDS_DHT_NETWORK_IMESSAGE_MAP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "messages/dht_route_messages.h"
#include "service_provider.h"
#include "const_data_buffer.h"
#include "dht_network_client.h"

namespace vds {
  namespace dht {
    namespace network {
      class dht_session;

      class imessage_map {
      public:
        class message_info_t {
        public:
          message_info_t() = default;

          message_info_t(
            const std::shared_ptr<dht_session>& session,
            message_type_t message_type,
            const const_data_buffer& message_data,
            const std::vector<const_data_buffer> & hops)
            : session_(session),
              message_type_(message_type),
              message_data_(message_data),
              hops_(hops) {
          }

          const std::shared_ptr<dht_session>& session() const {
            return session_;
          }

          message_type_t message_type() const {
            return message_type_;
          }

          const const_data_buffer& message_data() const {
            return message_data_;
          }

          const const_data_buffer& source_node() const {
            return this->hops_[this->hops_.size() - 1];
          }

          const const_data_buffer& last_proxy_node() const {
            return this->hops_[0];
          }

          const std::vector<const_data_buffer>& hops() const {
            return hops_;
          }

        private:
          std::shared_ptr<dht_session> session_;
          message_type_t message_type_;
          const_data_buffer message_data_;
          std::vector<const_data_buffer> hops_;
        };

        virtual vds::async_task<vds::expected<bool>> process_message(
          message_info_t message_info) = 0;

        virtual vds::async_task<vds::expected<void>> on_new_session(
          const_data_buffer partner_id) = 0;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_IMESSAGE_MAP_H_
