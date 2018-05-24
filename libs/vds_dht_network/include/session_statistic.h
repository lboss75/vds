#ifndef __VDS_DHT_NETWORK_SESSION_STATISTIC_H_
#define __VDS_DHT_NETWORK_SESSION_STATISTIC_H_
#include "json_object.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include "encoding.h"

namespace vds {
  struct session_statistic {

    struct session_info {
      std::string address_;
      unsigned int offer_replica_;
      unsigned int replica_request_;
      unsigned int dht_pong_;
      unsigned int dht_ping_;
      unsigned int dht_find_node_response_;
      unsigned int dht_find_node_;
      unsigned int transaction_log_record_;
      unsigned int transaction_log_request_;
      unsigned int transaction_log_state_count_;

      void serialize(std::shared_ptr<json_array> & items) const {
        auto result = std::make_shared<json_object>();
        result->add_property("address", this->address_);
        result->add_property("offer_replica", this->offer_replica_);
        result->add_property("replica_request", this->replica_request_);
        result->add_property("dht_pong", this->dht_pong_);
        result->add_property("dht_ping", this->dht_ping_);
        result->add_property("dht_find_node_response", this->dht_find_node_response_);
        result->add_property("dht_find_node", this->dht_find_node_);
        result->add_property("transaction_log_record", this->transaction_log_record_);
        result->add_property("transaction_log_request", this->transaction_log_request_);
        result->add_property("transaction_log_state", this->transaction_log_state_count_);
        items->add(result);
      }
    };

    std::list<session_info> items_;

    std::shared_ptr<json_value> serialize() const {
      auto result = std::make_shared<json_object>();

      auto items = std::make_shared<json_array>();
      for(const auto & p : this->items_) {
        p.serialize(items);
      }

      result->add_property("items", items);

      return result;
    }
  };
}

#endif //__VDS_DHT_NETWORK_SESSION_STATISTIC_H_