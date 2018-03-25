#ifndef __VDS_P2P_NETWORK_OFFER_MOVE_REPLICA_H_
#define __VDS_P2P_NETWORK_OFFER_MOVE_REPLICA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

namespace vds {
  namespace dht {
    namespace messages {
      class offer_move_replica {
      public:
        static const network::message_type_t message_id = network::message_type_t::offer_move_replica;

        offer_move_replica(
          const const_data_buffer & object_id,
          const uint16_t replica,
          const const_data_buffer & target_node,
          const const_data_buffer & source_node)
          : object_id_(object_id),
            replica_(replica),
            target_node_(target_node),
            source_node_(source_node) {
        }

        offer_move_replica(
          binary_deserializer & s) {
          s >> this->object_id_ >> this->replica_ >> this->target_node_ >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->object_id_ << this->replica_ << this->target_node_ << this->source_node_;
          return s.data();
        }

        const const_data_buffer &object_id() const {
          return object_id_;
        }

        uint16_t replica() const {
          return this->replica_;
        }

        const const_data_buffer & target_node() const {
          return target_node_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer object_id_;
        uint16_t replica_;
        const_data_buffer target_node_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_P2P_NETWORK_OFFER_MOVE_REPLICA_H_
