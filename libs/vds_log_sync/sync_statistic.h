#ifndef __VDS_LOG_SYNC_SYNC_STATISTIC_H_
#define __VDS_LOG_SYNC_SYNC_STATISTIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  struct sync_statistic {

    std::list<std::string> leafs_;


    std::string str() const {
      std::string result;
      for(auto & p : this->leafs_){
        if(!result.empty()){
          result += ',';
        }

        result += p;
      }

      return result;
    }

    operator bool () const {
      return !this->leafs_.empty();
    }

    bool operator != (const sync_statistic & other) const {
      if(this->leafs_.size() != other.leafs_.size()){
        return true;
      }

      for(auto & p : this->leafs_){
        bool is_good = false;
        for(auto & p1 : other.leafs_){
          if(p == p1){
            is_good = true;
            break;
          }
        }
        if(!is_good){
          return true;
        }
      }
      return true;
    }
  };
}

#endif //__VDS_LOG_SYNC_SYNC_STATISTIC_H_
