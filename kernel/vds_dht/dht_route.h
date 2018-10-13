#ifndef __VDS_DHT_NETWORK_P2P_ROUTE_H_
#define __VDS_DHT_NETWORK_P2P_ROUTE_H_

#include "const_data_buffer.h"
#include "logger.h"
#include "dht_object_id.h"
#include "legacy.h"
#include <map>

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  struct route_statistic;

  namespace dht {
    template<typename session_type>
    class dht_route {
    public:
      struct node : public std::enable_shared_from_this<node>{
        const_data_buffer node_id_;
        session_type proxy_session_;
        uint8_t pinged_;
        uint8_t hops_;

        node()
            : pinged_(0) {
        }

        node(
            const const_data_buffer &id,
            const session_type &proxy_session,
            uint8_t hops)
            : node_id_(id),
              proxy_session_(proxy_session),
              hops_(hops),
              pinged_(0) {
        }
        node(node && origin)
          : node_id_(std::move(origin.node_id_)),
          proxy_session_(std::move(origin.proxy_session_)),
          hops_(origin.hops_),
          pinged_(origin.pinged_) {
        }
        node(const node & origin)
          : node_id_(origin.node_id_),
          proxy_session_(origin.proxy_session_),
          hops_(origin.hops_),
          pinged_(origin.pinged_) {
        }

        bool is_good() const {
          return this->pinged_ < 10;
        }

        void reset(
            const const_data_buffer &id,
            const session_type &proxy_session,
            uint8_t hops) {
          this->node_id_ = id;
          this->proxy_session_ = proxy_session;
          this->hops_ = hops;
          this->pinged_ = 0;
        }
      };

      dht_route(
        const service_provider * sp,
        const const_data_buffer &this_node_id)
          : sp_(sp), current_node_id_(this_node_id) {

      }

      const const_data_buffer &current_node_id() const {
        return this->current_node_id_;
      }

      bool add_node(
          
          const const_data_buffer &id,
          const session_type &proxy_session,
          uint8_t hops,
          bool allow_skip) {
        vds_assert(id != this->current_node_id_);

        const auto index = dht_object_id::distance_exp(this->current_node_id_, id);
        std::shared_ptr<bucket> b;

        std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
        auto p = this->buckets_.find(index);
        if (this->buckets_.end() == p) {
          lock.unlock();

          std::unique_lock<std::shared_mutex> ulock(this->buckets_mutex_);
          auto p = this->buckets_.find(index);
          if (this->buckets_.end() == p) {
            b = std::make_shared<bucket>();
            this->buckets_[index] = b;
          } else {
            b = p->second;
          }
        } else {
          b = p->second;
          lock.unlock();
        }

        return b->add_node(id, proxy_session, hops, allow_skip);
      }

      template <typename... timer_arg_types>
      vds::async_task<void> on_timer(
          
          timer_arg_types && ... timer_args) {
        return this->ping_buckets(std::forward<timer_arg_types>(timer_args)...);
      }

      void neighbors(
          
          const const_data_buffer &target_id,
          std::map<vds::const_data_buffer /*distance*/, std::list<vds::const_data_buffer/*node_id*/>> &result,
          uint16_t max_count) const {

        std::map<vds::const_data_buffer /*distance*/, std::map<vds::const_data_buffer, std::shared_ptr<node>>> tmp;

        this->search_nodes(target_id, max_count, tmp);

        uint16_t count = 0;
        for (auto &p : tmp) {
          auto &presult = result[p.first];
          for (auto & pnode : p.second) {
            presult.push_back(pnode.first);
            ++count;
          }
          if (count > max_count) {
            break;
          }
        }
      }

      void search_nodes(
        
        const const_data_buffer &target_id,
        size_t max_count,
        std::map<const_data_buffer /*distance*/, std::map<const_data_buffer, std::shared_ptr<node>>> &result_nodes) const {
        std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
        this->_search_nodes(target_id, max_count, result_nodes);
      }

      void search_nodes(
        
        const const_data_buffer &target_id,
        size_t max_count,
        const std::function<bool(const node & node)>& filter,
        std::map<const_data_buffer /*distance*/, std::map<const_data_buffer, std::shared_ptr<node>>> &result_nodes) const {
        std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
        this->_search_nodes(target_id, max_count, filter, result_nodes);
      }

      vds::async_task<void> for_near(
        
        const const_data_buffer &target_node_id,
        size_t max_count,
        const std::function<async_task<bool>(const std::shared_ptr<node> & candidate)> &callback) {

        std::map<
            const_data_buffer /*distance*/,
            std::map<const_data_buffer, std::shared_ptr<node>>> result_nodes;
        this->search_nodes(target_node_id, max_count, result_nodes);

        for (auto &presult : result_nodes) {
          for (auto & node : presult.second) {
            if (!co_await callback(node.second)) {
              co_return;
            }
          }
        }
      }

      vds::async_task<void> for_near(
        
        const const_data_buffer &target_node_id,
        size_t max_count,
        const std::function<bool(const node & node)>& filter,
        const std::function<vds::async_task<bool>(const std::shared_ptr<node> & candidate)> &callback) {

        std::map<
            const_data_buffer /*distance*/,
            std::map<const_data_buffer, std::shared_ptr<node>>> result_nodes;
        this->search_nodes(target_node_id, max_count, filter, result_nodes);

        for (auto &presult : result_nodes) {
          for (auto & node : presult.second) {
            if (!co_await callback(node.second)) {
              co_return;
            }
          }
        }
      }
      

      void get_neighbors(
        
        std::list<std::shared_ptr<node>> & result_nodes) const;

      vds::async_task<void> for_neighbors(
        
        const std::function<vds::async_task<bool>(const std::shared_ptr<node> & candidate)> &callback) {

        std::list<std::shared_ptr<node>> result_nodes;
        this->get_neighbors(result_nodes);

        for (auto & node : result_nodes) {
          if (!co_await callback(node)) {
            co_return;
          }
        }
      }

      void mark_pinged(const const_data_buffer& target_node, const network_address& address) {
        auto index = dht_object_id::distance_exp(this->current_node_id_, target_node);

        std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
        auto p = this->buckets_.find(index);
        if(this->buckets_.end() != p) {
          p->second->mark_pinged(target_node, address);
        }
      }

      void get_statistics(route_statistic& result);
      void remove_session(
        
        const session_type & session) {

        std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
        for(auto p : this->buckets_){
          p.second->remove_session(session);
        }
      }

    private:
      const service_provider * sp_;
      const_data_buffer current_node_id_;

      struct bucket : public std::enable_shared_from_this<bucket> {
        static constexpr size_t MAX_NODES = 8;

        mutable std::shared_mutex nodes_mutex_;
        std::list<std::shared_ptr<node>> nodes_;

        bool add_node(
            
            const const_data_buffer &id,
            const session_type &proxy_session,
            uint8_t hops,
            bool allow_skip) {

          std::unique_lock<std::shared_mutex> ulock(this->nodes_mutex_);
          for (const auto &p : this->nodes_) {
            if (p->node_id_ == id && p->proxy_session_->address() == proxy_session->address()) {
              return false;//Already exists
            }
          }

          if (!allow_skip || 0 == hops || MAX_NODES > this->nodes_.size()) {
            this->nodes_.push_back(std::make_shared<node>(id, proxy_session, hops));
            return true;
          }

          for (auto &p : this->nodes_) {
            if (!p->is_good()) {
              p->reset(id, proxy_session, hops);
              return true;
            }
          }

          return false;
        }

        void remove_session(
          const session_type & proxy_session) {
          std::unique_lock<std::shared_mutex> ulock(this->nodes_mutex_);
          for(auto p = this->nodes_.begin(); p != this->nodes_.end(); ){
            if ((*p)->proxy_session_->address() == proxy_session->address()) {
              p = this->nodes_.erase(p);
            }
            else {
              ++p;
            }
          }
        }

        template <typename... timer_arg_types>
        vds::async_task<void> on_timer(
            const service_provider * sp,
            const dht_route *owner,
            timer_arg_types && ... timer_args) {
          std::list<std::tuple<const_data_buffer, session_type>> sessions;
          
          this->nodes_mutex_.lock();
          for (auto p : this->nodes_) {
            sp->get<logger>()->trace("DHT", "Bucket node node_id=%s,proxy_session=%s,pinged=%d,hops=%d",
              base64::from_bytes(p->node_id_).c_str(),
              p->proxy_session_->address().to_string().c_str(),
              p->pinged_,
              p->hops_);

            p->pinged_++;
            sessions.push_back({ p->node_id_, p->proxy_session_ });
          }
          this->nodes_mutex_.unlock();

          for (const auto & s : sessions) {
            co_await std::get<1>(s)->ping_node(
              std::get<0>(s),
              std::forward<timer_arg_types>(timer_args)...);
          }
        }

        bool contains(const const_data_buffer &node_id) const {
          std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
          for (auto &p : this->nodes_) {
            if (p->node_id_ == node_id) {
              return true;
            }
          }

          return false;
        }

        void mark_pinged(const const_data_buffer& target_node, const network_address& address) {
          std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
          for (auto &p : this->nodes_) {
            if (p->node_id_ == target_node && p->proxy_session_->address() == address) {
              p->pinged_ = 0;
              break;
            }
          }
        }

        void get_statistics(route_statistic& result) {
          std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
          for (auto &p : this->nodes_) {
            result.items_.push_back(
              route_statistic::route_info{
                p->node_id_,
                p->proxy_session_->address().to_string(),
                p->pinged_,
                p->hops_
              }
            );
          }
        }

        void get_neighbors( std::list<std::shared_ptr<node>> & result_nodes) const {
          std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
          for (auto & p : this->nodes_) {
            if (p->hops_ == 0) {
              result_nodes.push_back(p);
            }
          }
        }
      };

      mutable std::shared_mutex buckets_mutex_;
      std::map<size_t, std::shared_ptr<bucket>> buckets_;

      void _search_nodes(
          
          const const_data_buffer &target_id,
          size_t max_count,
          std::map<const_data_buffer /*distance*/, std::map<const_data_buffer, std::shared_ptr<node>>> &result_nodes) const {

        if (this->buckets_.empty()) {
          return;
        }

        auto index = dht_object_id::distance_exp(this->current_node_id_, target_id);

        auto min_index = this->buckets_.begin()->first;
        auto max_index = this->buckets_.rbegin()->first;

        size_t count = 0;
        for (
            size_t distance = 0;
            result_nodes.size() < max_count
            && (index + distance <= max_index || (index >= distance && index - distance >= min_index));
            ++distance) {
          if (index + distance <= max_index) {
            count += this->looking_nodes(target_id, [](const node &)->bool {return true; }, result_nodes, index + distance);
          }
          if (index >= distance && index - distance >= min_index) {
            count += this->looking_nodes(target_id, [](const node &)->bool {return true; }, result_nodes, index - distance);
          }
          if (count > max_count) {
            break;
          }
        }
      }

      void _search_nodes(
        
        const const_data_buffer &target_id,
        size_t max_count,
        const std::function<bool(const node & node)>& filter,
        std::map<const_data_buffer /*distance*/, std::map<const_data_buffer, std::shared_ptr<node>>> &result_nodes) const {

        if (this->buckets_.empty()) {
          return;
        }

        auto index = dht_object_id::distance_exp(this->current_node_id_, target_id);

        auto min_index = this->buckets_.begin()->first;
        auto max_index = this->buckets_.rbegin()->first;

        size_t count = 0;
        for (
          size_t distance = 0;
          result_nodes.size() < max_count
          && (index + distance <= max_index || (index >= distance && index - distance >= min_index));
          ++distance) {
          if (index + distance <= max_index) {
            count += this->looking_nodes(target_id, filter, result_nodes, index + distance);
          }
          if (index >= distance && index - distance >= min_index) {
            count += this->looking_nodes(target_id, filter, result_nodes, index - distance);
          }
          if (count > max_count) {
            break;
          }
        }
      }

      size_t looking_nodes(
          const const_data_buffer &target_id,
          const std::function<bool(const node & node)>& filter,
          std::map<const_data_buffer, std::map<const_data_buffer, std::shared_ptr<node>>> &result_nodes,
          size_t index) const {
        size_t result = 0;
        auto p = this->buckets_.find(index);
        if (this->buckets_.end() == p) {
          return result;
        }

        std::shared_lock<std::shared_mutex> lock(p->second->nodes_mutex_);

        for (const auto & node : p->second->nodes_) {
          if (!node->is_good() || !filter(*node)) {
            continue;
          }

          auto & result_node = result_nodes[dht_object_id::distance(node->node_id_, target_id)];
          auto exists = result_node.find(node->node_id_);
          if(result_node.end() == exists){
            result_node[node->node_id_] = node;
            ++result;
          }
          else {
            if (exists->second->hops_ > node->hops_) {
              exists->second = node;
            }
          }
        }

        return result;
      }

      template <typename... timer_arg_types>
      vds::async_task<void> ping_buckets( timer_arg_types && ... timer_args) {
        std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
        for (auto &p : this->buckets_) {
          logger::get(this->sp_)->trace("DHT", "Bucket %d", p.first);
          co_await p.second->on_timer(this->sp_, this, timer_args...);
        }
      }
    };

    template <typename session_type>
    void dht_route<session_type>::get_neighbors(
      std::list<std::shared_ptr<node>>& result_nodes) const {
      std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
      for (auto &p : this->buckets_) {
        p.second->get_neighbors(result_nodes);
      }
    }

    template <typename session_type>
    void dht_route<session_type>::get_statistics(route_statistic& result) {
      result.node_id_ = this->current_node_id();
      std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
      for (auto &p : this->buckets_) {
        p.second->get_statistics(result);
      }
    }
  }
}



#endif //__VDS_DHT_NETWORK_P2P_ROUTE_H_
