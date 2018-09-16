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
      bool blocked_;
      bool not_started_;

      void serialize(std::shared_ptr<json_array>& items) const {
        auto result = std::make_shared<json_object>();
        result->add_property("address", this->address_);
        if(this->blocked_) {
          result->add_property("blocked", "true");
        }
        if(this->not_started_) {
          result->add_property("not_started", "true");
        }
        items->add(result);
      }
    };

    std::list<session_info> items_;

    std::shared_ptr<json_value> serialize() const {
      auto result = std::make_shared<json_object>();

      auto items = std::make_shared<json_array>();
      for (const auto& p : this->items_) {
        p.serialize(items);
      }

      result->add_property("items", items);

      return result;
    }
  };
}

#endif //__VDS_DHT_NETWORK_SESSION_STATISTIC_H_
