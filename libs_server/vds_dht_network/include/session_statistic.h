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


    struct traffic_info {
      traffic_info()
        : sent_count_(0), sent_(0),
          good_count_(0), good_traffic_(0),
          bad_count_(0), bad_traffic_(0) {
      }

      uint64_t sent_count_;
      uint64_t sent_;

      uint64_t good_count_;
      uint64_t good_traffic_;

      uint64_t bad_count_;
      uint64_t bad_traffic_;
    };

    struct session_info {
      std::string address_;
      bool blocked_;
      bool not_started_;

      std::map<std::string /*from*/, std::map<std::string /*to*/, std::map<uint8_t /*message_type*/, traffic_info /*size*/>>> traffic;

      void serialize(std::shared_ptr<json_array>& items) const {
        auto result = std::make_shared<json_object>();
        result->add_property("address", this->address_);
        if(this->blocked_) {
          result->add_property("blocked", "true");
        }
        if(this->not_started_) {
          result->add_property("not_started", "true");
        }

        uint64_t sent = 0;
        uint64_t sent_count = 0;
        uint64_t good_traffic = 0;
        uint64_t good_count = 0;
        uint64_t bad_traffic = 0;
        uint64_t bad_count = 0;

        auto traffic_result = std::make_shared<json_array>();
        for (const auto & item : this->traffic)
        {
          auto item_result = std::make_shared<json_object>();
          item_result->add_property("from", item.first);

          auto item_result_items = std::make_shared<json_array>();

          for (const auto & to_item : item.second)
          {
            auto to_result = std::make_shared<json_object>();
            to_result->add_property("to", to_item.first);

            auto to_result_items = std::make_shared<json_array>();

            for (const auto & message_item : to_item.second)
            {
              auto message_result = std::make_shared<json_object>();
              message_result->add_property("msg", message_item.first);
              message_result->add_property("sent", message_item.second.sent_);
              message_result->add_property("sent_count", message_item.second.sent_count_);
              message_result->add_property("rcv_good", message_item.second.good_traffic_);
              message_result->add_property("rcv_good_count", message_item.second.good_count_);
              message_result->add_property("rcv_bad", message_item.second.bad_traffic_);
              message_result->add_property("rcv_bad_count", message_item.second.bad_count_);
              to_result_items->add(message_result);

              sent += message_item.second.sent_;
              sent_count += message_item.second.sent_count_;
              good_traffic += message_item.second.good_traffic_;
              good_count += message_item.second.good_count_;
              bad_traffic += message_item.second.bad_traffic_;
              bad_count += message_item.second.bad_count_;
            }

            to_result->add_property("messages", to_result_items);
            item_result_items->add(to_result);
          }
          item_result->add_property("to", item_result_items);

          traffic_result->add(item_result);
        }
        result->add_property("traffic", traffic_result);
        result->add_property("total_sent", sent);
        result->add_property("total_sent_count", sent_count);
        result->add_property("good_traffic", good_traffic);
        result->add_property("good_count", good_count);
        result->add_property("bad_traffic", bad_traffic);
        result->add_property("bad_count", bad_count);

        items->add(result);
      }
    };

    size_t output_size_;
    std::list<session_info> items_;

    std::shared_ptr<json_value> serialize() const {
      auto result = std::make_shared<json_object>();
      
      result->add_property("output_size", std::to_string(this->output_size_));

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
