#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_STATISTIC_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_STATISTIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class p2p_network_statistic {
  public:
    struct bucket_info {
      std::list<std::string> nodes_;
    };
    std::string this_node_;
    std::map<int, bucket_info> buckets_;

    std::string str() const {
      std::string result = "This node is ";
      result += this->this_node_;
      result += ';';
      for(const auto & p : this->buckets_) {
        result += '[';
        result += std::to_string(p.first);
        result += "]:";
        for (const auto & node : p.second.nodes_) {
          result += node;
          result += ',';
        }
        result += ';';
      }
      return result;
    }
  };
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_STATISTIC_H_
