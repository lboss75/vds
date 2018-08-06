#ifndef __VDS_DHT_NETWORK_ROUTE_STATISTIC_H_
#define __VDS_DHT_NETWORK_ROUTE_STATISTIC_H_
#include "json_object.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include "encoding.h"

namespace vds {
  struct route_statistic {

    struct route_info {
      const_data_buffer node_id_;
      std::string proxy_address_;
      size_t outgoing_queue_size_;
      uint8_t pinged_;
      uint8_t hops_;

      void serialize(bool include_invalid, std::shared_ptr<json_array> & items) const {
        if(include_invalid || pinged_ < 10) {
          auto result = std::make_shared<json_object>();
          result->add_property("node_id", base64::from_bytes(this->node_id_));
          result->add_property("proxy", this->proxy_address_);
          result->add_property("outgoing_queue_size", this->outgoing_queue_size_);
          result->add_property("pinged", this->pinged_);
          result->add_property("hops", this->hops_);
          items->add(result);
        }
      }
    };

    std::list<route_info> items_;
    const_data_buffer node_id_;

    std::shared_ptr<json_value> serialize(bool include_invalid = false) const {
      auto result = std::make_shared<json_object>();
      result->add_property("node_id", base64::from_bytes(this->node_id_));

      auto items = std::make_shared<json_array>();
      for(const auto & p : items_) {
        p.serialize(include_invalid, items);
      }

      result->add_property("items", items);

      return result;
    }
  };
}

#endif //__VDS_DHT_NETWORK_ROUTE_STATISTIC_H_