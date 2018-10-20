#ifndef __VDS_DHT_NETWORK_DHT_ROUTE_MESSAGES_H_
#define __VDS_DHT_NETWORK_DHT_ROUTE_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace network {

      enum class protocol_message_type_t {
        HandshakeBroadcast = 0,
        Handshake = 1,
        Welcome = 2,

        ContinueData = 3,

        Failed = 5,
        
        MTUTest = 6,
        MTUTestPassed = 7,

        SpecialCommand = 0b11100000,

        SingleData = 0b00100000,
        Data = 0b01000000,

        RouteSingleData = 0b01100000,
        RouteData = 0b10000000,

        ProxySingleData = 0b10100000,
        ProxyData = 0b11000000,
      };

      enum class message_type_t {
        transaction_log_state,
        transaction_log_request,
        transaction_log_record,

        dht_find_node,
        dht_find_node_response,
        dht_ping,
        dht_pong,

        sync_new_election_request,
        sync_new_election_response,

        sync_add_message_request,

        sync_leader_broadcast_request,
        sync_leader_broadcast_response,

        sync_replica_operations_request,
        sync_replica_operations_response,

        sync_looking_storage_request,
        sync_looking_storage_response,

        sync_snapshot_request,
        sync_snapshot_response,

        sync_offer_send_replica_operation_request,
        sync_offer_remove_replica_operation_request,
        //sync_offer_replica_operation_response,

        sync_replica_request,
        sync_replica_data,

        sync_replica_query_operations_request

      };
    }

    namespace messages {

      class dht_ping {
      public:
        static const network::message_type_t message_id = network::message_type_t::dht_ping;

        template <typename visitor_type>
        void visit(visitor_type & v) {
          v();
        }
      };

      class dht_pong {
      public:
        static const network::message_type_t message_id = network::message_type_t::dht_pong;

        template <typename visitor_type>
        void visit(visitor_type & v) {
          v();
        }
      };

      class dht_find_node {
      public:
        static const network::message_type_t message_id = network::message_type_t::dht_find_node;

        const_data_buffer target_id;

        template <typename visitor_type>
        void visit(visitor_type & v) {
          v(this->target_id);
        }

      };

      class dht_find_node_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::dht_find_node_response;

        struct target_node {
          const_data_buffer target_id_;
          std::string address_;
          uint8_t hops_;

          target_node() {
          }

          target_node(const target_node &) = default;

          target_node(target_node && other)
          : target_id_(std::move(other.target_id_)),
            address_(std::move(other.address_)),
            hops_(other.hops_) {
          }

          target_node(
            const const_data_buffer& target_id,
            const std::string& address,
            uint8_t hops)
            : target_id_(target_id), address_(address), hops_(hops) {
          }

          target_node & operator = (const target_node & other) {
            this->target_id_ = other.target_id_;
            this->address_ = other.address_;
            this->hops_ = other.hops_;

            return *this;
          }

          target_node & operator = (target_node && other) {
            this->target_id_ = std::move(other.target_id_);
            this->address_ = std::move(other.address_);
            this->hops_ = other.hops_;

            return *this;
          }
        };

        std::list<target_node> nodes;

        template <typename visitor_type>
        void visit(visitor_type & v) {
          v(this->nodes);
        }
      };
    }
  }

  inline binary_serializer& operator <<(
    binary_serializer& s,
    const dht::messages::dht_find_node_response::target_node& node) {
    return s << node.target_id_ << node.address_ << node.hops_;
  }

  inline binary_deserializer& operator >>(
    binary_deserializer& s,
    dht::messages::dht_find_node_response::target_node& node) {
    return s >> node.target_id_ >> node.address_ >> node.hops_;
  }
}

namespace std {
#define enum2str(x) case vds::dht::network::message_type_t::x: return #x;
  inline std::string to_string(vds::dht::network::message_type_t v) {
    switch (v) {
      enum2str(transaction_log_state);
      enum2str(transaction_log_request);
      enum2str(transaction_log_record);
      enum2str(dht_find_node);
      enum2str(dht_find_node_response);
      enum2str(dht_ping);
      enum2str(dht_pong);
      enum2str(sync_new_election_request);
      enum2str(sync_new_election_response);
      enum2str(sync_add_message_request);
      enum2str(sync_leader_broadcast_request);
      enum2str(sync_leader_broadcast_response);
      enum2str(sync_replica_operations_request);
      enum2str(sync_replica_operations_response);
      enum2str(sync_looking_storage_request);
      enum2str(sync_looking_storage_response);
      enum2str(sync_snapshot_request);
      enum2str(sync_snapshot_response);
      enum2str(sync_offer_send_replica_operation_request);
      enum2str(sync_offer_remove_replica_operation_request);
      enum2str(sync_replica_request);
      enum2str(sync_replica_data);
      enum2str(sync_replica_query_operations_request);
    default:
      return "unknown";
    }
  }

#undef enum2str

}

#endif//__VDS_DHT_NETWORK_DHT_ROUTE_MESSAGES_H_