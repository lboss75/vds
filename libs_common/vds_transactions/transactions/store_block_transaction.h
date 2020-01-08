#ifndef __VDS_TRANSACTIONS_STORE_BLOCK_TRANSACTION_H_
#define __VDS_TRANSACTIONS_STORE_BLOCK_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "asymmetriccrypto.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "transaction_id.h"

namespace vds {
  namespace transactions {
    class store_block_transaction {
    public:
      static const transaction_id message_id = transaction_id::store_block_transaction;

      const_data_buffer owner_id;
      const_data_buffer object_id;
      uint64_t object_size;
      uint32_t replica_size;
      std::vector<const_data_buffer> replicas;
      const_data_buffer owner_sig;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          owner_id,
          object_id,
          object_size,
          replica_size,
          replicas,
          owner_sig
        );
      }

      static expected<store_block_transaction> create(
        const const_data_buffer& owner_id,
        const const_data_buffer& object_id,
        uint64_t object_size,
        uint32_t replica_size,
        const std::vector<const_data_buffer>& replicas,
        const asymmetric_private_key& private_key) {

        binary_serializer s;
        CHECK_EXPECTED(s << owner_id);
        CHECK_EXPECTED(s << object_id);
        CHECK_EXPECTED(s << object_size);
        CHECK_EXPECTED(s << replica_size);
        CHECK_EXPECTED(s << replicas);

        GET_EXPECTED(owner_sig, asymmetric_sign::signature(hash::sha256(), private_key, s.move_data()));
        return message_create<store_block_transaction>(
          owner_id,
          object_id,
          object_size,
          replica_size,
          replicas,
          owner_sig);
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_STORE_BLOCK_TRANSACTION_H_
