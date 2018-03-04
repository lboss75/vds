#ifndef __VDS_LOG_SYNC_SYNC_STATISTIC_H_
#define __VDS_LOG_SYNC_SYNC_STATISTIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  struct sync_statistic {

    std::map<guid, std::list<std::string>> leafs_;

    struct record_info {
      std::string name_;
      std::string state_;
      std::list<std::shared_ptr<record_info>> children_;
    };
    std::list<std::shared_ptr<record_info>> roots_;

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
      result += '\n';

      std::set<const record_info *> processed;
      for(const auto & r : this->roots_) {
        this->print_node(result, r.get(), 0, processed);
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

#endif //__VDS_LOG_SYNC_SYNC_STATISTIC_H_
