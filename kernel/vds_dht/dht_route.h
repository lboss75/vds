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

  namespace dht {

    template<typename session_type>
    class dht_route {
    public:
      struct node {
        const_data_buffer node_id_;
        session_type proxy_session_;
        uint8_t pinged_;

        node()
          : pinged_(0) {
        }

        node(
          const const_data_buffer & id,
          const session_type & proxy_session)
          : node_id_(id),
          proxy_session_(proxy_session),
          pinged_(0) {
        }

        bool is_good() const {
          return this->pinged_ < 10;
        }

        void reset(
          const const_data_buffer & id,
          const session_type & proxy_session) {
          this->node_id_ = id;
          this->proxy_session_ = proxy_session;
          this->pinged_ = 0;
        }
      };

      dht_route(const const_data_buffer & this_node_id)
        : current_node_id_(this_node_id) {

      }

      void add_node(
        const vds::service_provider &sp,
        const const_data_buffer & id,
        const session_type & proxy_session) {

        const auto index = dht_object_id::distance_exp(this->current_node_id_, id);
        bucket * b;

        std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
        auto p = this->buckets_.find(index);
        if (this->buckets_.end() == p) {
          lock.unlock();

          std::unique_lock<std::shared_mutex> ulock(this->buckets_mutex_);
          auto p = this->buckets_.find(index);
          if (this->buckets_.end() == p) {
            b = &this->buckets_[index];
          }
          else {
            b = &p->second;
          }
        }
        else {
          b = &p->second;
          lock.unlock();
        }

        b->add_node(sp, id, proxy_session);
      }

      void on_timer(
        const service_provider &sp) {
        this->ping_buckets(sp);
        this->update_route_table(sp);
      }

    private:
      const_data_buffer current_node_id_;

      struct bucket {
        static constexpr size_t MAX_NODES = 8;

        mutable std::shared_mutex nodes_mutex_;
        std::list<node> nodes_;

        void add_node(
          const service_provider &sp,
          const const_data_buffer & id,
          const session_type & proxy_session) {

          std::unique_lock<std::shared_mutex> ulock(this->nodes_mutex_);
          for (const auto & p : this->nodes_) {
            if (p.id_ == id && p.proxy_session_->address() == proxy_session->address()) {
              return;//Already exists
            }
          }

          if (MAX_NODES > this->nodes_.size()) {
            this->nodes_.push_back(node(id, proxy_session));
            return;
          }

          for (auto & p : this->nodes_) {
            if (!p.is_good()) {
              p.reset(id, proxy_session);
              return;
            }
          }
        }

        void on_timer(
          const service_provider &sp,
          const dht_route * owner) {

          std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
          for (auto & p : this->nodes_) {
            p.proxy_session_->send(
              sp,
              p.id_,
              p2p_messages::dht_ping(owner->current_node_id().device_id()).serialize());
            p.pinged_++;
          }
        }

        bool contains(const const_data_buffer &node_id) const {
          std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
          for (auto & p : this->nodes_) {
            if (p.id_ == node_id) {
              return true;
            }
          }

          return false;
        }

      };

      mutable std::shared_mutex buckets_mutex_;
      std::map<size_t, bucket> buckets_;

      void _search_nodes(
        const vds::service_provider &sp,
        const const_data_buffer & target_id,
        size_t max_count,
        std::list<node> & result_nodes) {

        auto index = dht_object_id::distance_exp(this->current_node_id_, target_id);

        std::map<vds::node_id_t, node> result;
        for (
          uint8_t distance = 0;
          result_nodes.size() < max_count
          && (index + distance < 8 * node_id_t::SIZE || index - distance >= 0);
          ++distance) {
          this->search_nodes(sp, target_id, result, index + distance);
          this->search_nodes(sp, target_id, result, index - distance);
        }

        for (auto & p : result) {
          result_nodes.push_back(p.second);
          if (result_nodes.size() >= max_count) {
            break;
          }
        }
      }

      void search_nodes(
        const service_provider &sp,
        const const_data_buffer &target_id,
        std::map<const_data_buffer, node> &result_nodes,
        uint8_t index) {
        auto p = this->buckets_.find(index);
        if (this->buckets_.end() == p) {
          return;
        }

        for (auto & node : p->second.nodes_) {
          if (!node.is_good()) {
            continue;
          }

          result_nodes[node.id_.distance(target_id)] = node;

          //std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
          //this->_search_nodes(sp, target_id, max_count, result_nodes);
        }

        void ping_buckets(const service_provider &sp) {
          std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
          for (auto & p : this->buckets_) {
            p.second.on_timer(sp, this);
          }
        }

        void update_route_table(const service_provider &sp) {
          for (uint8_t i = 0; i < 8 * node_id_t::SIZE; ++i) {
            for (;;) {
              auto canditate = this->current_node_id_.generate_random_id(i);

              std::unique_lock<std::shared_mutex> lock(this->buckets_mutex_);
              auto p = this->buckets_.find(i);
              if (this->buckets_.end() == p || !p->second.contains(canditate)) {
                this->find_node(sp, canditate);
                break;
              }
            }
          }
        }

        void find_node(
          const service_provider &sp,
          const const_data_buffer &node_id) {
          this->for_near(
            sp,
            target_node_id,
            40,
            [sp, target_node_id](const node_id_t & node_id, const std::shared_ptr<vds::_p2p_crypto_tunnel> & proxy_session) {
            sp.get<logger>()->trace(ThisModule, sp, "DHT find node %s over %s (%s)",
              target_node_id.device_id().str().c_str(),
              node_id.device_id().str().c_str(),
              proxy_session->address().to_string().c_str());
            proxy_session->send(
              sp,
              node_id,
              p2p_messages::dht_find_node(target_node_id).serialize());

          });
        }

        void for_near(
          const service_provider & sp,
          const const_data_buffer & target_node_id,
          size_t max_count,
          const std::function<void(
            const const_data_buffer & node_id,
            const session_type & session)> & callback) {

          std::list<node> result_nodes;
          this->_search_nodes(sp, target_node_id, max_count, result_nodes);

          for (auto & node : result_nodes) {
            auto index = dht_object_id::distance_exp(this->current_node_id_, node.id_);
            auto p = this->buckets_.find(index);
            if (this->buckets_.end() != p) {
              std::shared_lock<std::shared_mutex> block(p->second.nodes_mutex_);
              for (auto & pnode : p->second.nodes_) {
                if (pnode.id_ == node.id_) {
                  callback(node.id_, pnode.proxy_session_);
                }
              }
            }
          }
        }
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_P2P_ROUTE_H_
