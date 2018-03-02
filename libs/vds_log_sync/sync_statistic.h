#ifndef __VDS_LOG_SYNC_SYNC_STATISTIC_H_
#define __VDS_LOG_SYNC_SYNC_STATISTIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  struct sync_statistic {

    std::map<guid, std::list<std::string>> leafs_;


    std::string str() const {
      std::string result;
      for (auto & c : this->leafs_) {
        result += c.first.str();
        result += '[';

        for (auto & p : c.second) {
          result += p;
          result += ',';
        }

        result += ']';
        result += ',';
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

      for (auto & c : this->leafs_) {
        auto oc = other.leafs_.find(c.first);
        if (other.leafs_.end() == oc) {
          return true;
        }

        for (auto & p : c.second) {
          bool is_good = false;
          for (auto & p1 : oc->second) {
            if (p == p1) {
              is_good = true;
              break;
            }
          }
          if (!is_good) {
            return true;
          }
        }
      }

      for (auto & oc : other.leafs_) {
        auto c = this->leafs_.find(oc.first);
        if (other.leafs_.end() == c) {
          return true;
        }

        for (auto & p : oc.second) {
          bool is_good = false;
          for (auto & p1 : c->second) {
            if (p == p1) {
              is_good = true;
              break;
            }
          }
          if (!is_good) {
            return true;
          }
        }
      }

      return false;
    }
  };
}

#endif //__VDS_LOG_SYNC_SYNC_STATISTIC_H_
