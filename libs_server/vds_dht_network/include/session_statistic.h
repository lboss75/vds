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

      void serialize(std::shared_ptr<json_object>& result) const {
        result->add_property("sent", this->sent_);
        result->add_property("sent_count", this->sent_count_);
        result->add_property("rcv_good", this->good_traffic_);
        result->add_property("rcv_good_count", this->good_count_);
        result->add_property("rcv_bad", this->bad_traffic_);
        result->add_property("rcv_bad_count", this->bad_count_);
      }
    };

    struct time_metric
    {
      time_t start_time_;
      time_t finish_time_;
      size_t mtu_;
      size_t output_queue_size_;
      size_t input_queue_size_;
      size_t idle_;
      size_t delay_;
      size_t service_traffic_;
      std::map<std::string /*from*/, std::map<std::string /*to*/, std::map<uint8_t /*message_type*/, traffic_info /*size*/>>> traffic_;

      void serialize(std::shared_ptr<json_array>& items) const {
        auto result = std::make_shared<json_object>();
        result->add_property("start", (uint64_t)this->start_time_);
        result->add_property("finish", (uint64_t)this->finish_time_);
        result->add_property("mtu", this->mtu_);
        result->add_property("output_queue", this->output_queue_size_);
        result->add_property("input_queue", this->input_queue_size_);
        result->add_property("idle", this->idle_);
        result->add_property("delay", this->delay_);
        result->add_property("service", this->service_traffic_);

        auto traffic_result = std::make_shared<json_array>();
        for (const auto& item : this->traffic_)
        {
          auto item_result = std::make_shared<json_object>();
          item_result->add_property("from", item.first);

          auto item_result_items = std::make_shared<json_array>();

          for (const auto& to_item : item.second)
          {
            auto to_result = std::make_shared<json_object>();
            to_result->add_property("to", to_item.first);

            auto to_result_items = std::make_shared<json_array>();

            for (const auto& message_item : to_item.second)
            {
              auto message_result = std::make_shared<json_object>();
              message_result->add_property("msg", message_item.first);
              message_item.second.serialize(message_result);
              to_result_items->add(message_result);
            }

            to_result->add_property("messages", to_result_items);
            item_result_items->add(to_result);
          }
          item_result->add_property("to", item_result_items);

          traffic_result->add(item_result);
        }
        result->add_property("traffic", traffic_result);
        
        items->add(result);
      }
    };

    struct session_info {
      std::string partner_;
      std::string address_;
      bool blocked_;
      bool not_started_;

      std::list<time_metric> metrics_;

      void serialize(std::shared_ptr<json_array>& items) const {
        auto result = std::make_shared<json_object>();
        result->add_property("partner", this->partner_);
        result->add_property("address", this->address_);
        if(this->blocked_) {
          result->add_property("blocked", "true");
        }
        if(this->not_started_) {
          result->add_property("not_started", "true");
        }

        auto metrics = std::make_shared<json_array>();
        for (const auto& m : this->metrics_) {
          m.serialize(metrics);
        }
        result->add_property("metrics", metrics);

        items->add(result);
      }
    };

    size_t send_queue_size_;
    std::list<session_info> items_;

    std::shared_ptr<json_value> serialize() const {
      auto result = std::make_shared<json_object>();
      
      result->add_property("send_queue_size", std::to_string(this->send_queue_size_));

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
