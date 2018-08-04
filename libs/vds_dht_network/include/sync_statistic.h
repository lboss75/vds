#ifndef __VDS_DHT_NETWORK_SYNC_STATISTIC_H_
#define __VDS_DHT_NETWORK_SYNC_STATISTIC_H_
#include "json_object.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  struct sync_statistic {

    struct log_info_t {
      const_data_buffer id;
      int state;
      uint64_t order_no;

      std::shared_ptr<json_value> serialize() const {
        auto result = std::make_shared<vds::json_object>();
        result->add_property("id", base64::from_bytes(this->id));
        result->add_property("state", this->state);
        result->add_property("order_no", this->order_no);

        return result;
      }
    };
    std::list<log_info_t> leafs_;
    std::set<const_data_buffer> unknown_;

    std::set<const_data_buffer> chunks_;
    std::map<const_data_buffer, std::set<uint16_t>> chunk_replicas_;

    struct record_info {
      std::string name_;
      std::string state_;
      std::list<std::shared_ptr<record_info>> children_;
    };
    std::list<std::shared_ptr<record_info>> roots_;

    operator bool () const {
      return !this->leafs_.empty();
    }

    bool operator != (const sync_statistic & other) const {
      if(this->leafs_.size() != other.leafs_.size()){
        return true;
      }

      for (auto & p : this->leafs_) {
        bool is_good = false;
        for (auto & p1 : other.leafs_) {
          if (p.id == p1.id) {
            is_good = true;
            break;
          }
        }
        if (!is_good) {
          return true;
        }
      }

      for (auto & p : other.leafs_) {
        bool is_good = false;
        for (auto & p1 : this->leafs_) {
          if (p.id == p1.id) {
            is_good = true;
            break;
          }
        }
        if (!is_good) {
          return true;
        }

      }

      return false;
    }

    std::shared_ptr<json_value> serialize() const {
      auto result = std::make_shared<vds::json_object>();

      auto leafs = std::make_shared<vds::json_array>();
      for(const auto & leaf : this->leafs_) {
        leafs->add(leaf.serialize());
      }
      result->add_property("leafs", leafs);

      auto unknowns = std::make_shared<vds::json_array>();
      for (const auto & unknown : this->unknown_) {
        unknowns->add(std::make_shared<json_primitive>(base64::from_bytes(unknown)));
      }
      result->add_property("unknowns", unknowns);

      return result;
    }
  private:
    static void print_node(std::string& result, const record_info * node, int level, std::set<const record_info *> & processed) {
      for(int i = 0; i < level; ++i) {
        result += '|';
      }
      result += node->name_;
      result += '(';
      result += node->state_;
      result += ')';
      if(processed.end() != processed.find(node)) {
        result += "See below\n";
      }
      else {
        processed.emplace(node);
        result += '\n';

        for (const auto & p : node->children_) {
          print_node(result, p.get(), level + 1, processed);
        }
      }
    }
  };
}

#endif //__VDS_DHT_NETWORK_SYNC_STATISTIC_H_