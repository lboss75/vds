#ifndef __VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_
#define __VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <set>
#include <map>
#include "channel_message.h"
#include "binary_serialize.h"
#include "database.h"
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"
#include "data_coin_balance.h"
#include "create_user_transaction.h"
#include "node_manager_transactions.h"

namespace vds {
  class _user_channel;

  namespace dht {
    namespace network {
      class _client;
    }
  }

  namespace transactions {
    class transaction_block_builder {
    public:
      transaction_block_builder() = default;
      transaction_block_builder(
        const service_provider * sp,
        const std::chrono::system_clock::time_point & time_point,
        const std::set<const_data_buffer> & ancestors,
        uint64_t order_no
      )
        : sp_(sp), time_point_(time_point), ancestors_(std::move(ancestors)), order_no_(order_no) {
      }

      static expected<transaction_block_builder> create(
        const service_provider * sp,
        class vds::database_read_transaction &t);

      static expected<transaction_block_builder> create(
        const service_provider * sp,
        class vds::database_read_transaction &t,
        const std::set<const_data_buffer> & ancestors);

      static transaction_block_builder create_root_block(const service_provider * sp) {
        return transaction_block_builder(sp);
      }
      
      expected<void> add(expected<create_user_transaction> && item);
      expected<void> add(expected<payment_transaction> && item);
      expected<void> add(expected<channel_message> && item);
      expected<void> add(expected<node_add_transaction> && item);

      expected<const_data_buffer> sign(
        const service_provider * sp,
        const std::shared_ptr<asymmetric_public_key> &write_public_key,
        const std::shared_ptr<asymmetric_private_key> &write_private_key);

    private:
        friend class _user_channel;
        friend class dht::network::_client;

        const service_provider * sp_;
        std::chrono::system_clock::time_point time_point_;
        std::set<const_data_buffer> ancestors_;
        uint64_t order_no_;
        binary_serializer data_;

        transaction_block_builder(const service_provider * sp);

        expected<const_data_buffer> save(
          const service_provider * sp,
          class vds::database_transaction &t,
          const std::shared_ptr<asymmetric_public_key> &write_public_key,
          const std::shared_ptr<asymmetric_private_key> &write_private_key);

    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_
