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
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"
#include "create_user_transaction.h"
#include "node_manager_transactions.h"
#include "store_block_transaction.h"

namespace vds {
  class _user_channel;

  namespace dht {
    namespace network {
      class client;
    }
  }

  namespace transactions {
    class transaction_block_builder {
    public:
      transaction_block_builder() = default;
      transaction_block_builder(transaction_block_builder &&) = default;

      transaction_block_builder & operator = (transaction_block_builder &&) = default;
      transaction_block_builder & operator = (const transaction_block_builder &) = delete;
      
      expected<void> add(expected<create_user_transaction> && item) {
        return add_transaction<create_user_transaction>(std::move(item));
      }
      expected<void> add(expected<store_block_transaction>&& item) {
        return add_transaction<store_block_transaction>(std::move(item));
      }
      expected<void> add(expected<create_wallet_transaction> && item) {
        return add_transaction<create_wallet_transaction>(std::move(item));
      }

      expected<void> add(expected<asset_issue_transaction> && item) {
        return add_transaction<asset_issue_transaction>(std::move(item));
      }

      expected<void> add(expected<payment_transaction> && item) {
        return add_transaction<payment_transaction>(std::move(item));
      }
      expected<void> add(expected<channel_message> && item) {
        return add_transaction<channel_message>(std::move(item));
      }
      expected<void> add(expected<node_add_transaction> && item) {
        return add_transaction<node_add_transaction>(std::move(item));
      }

      const_data_buffer close();

      expected<void> push_data(const const_data_buffer& data) {
        return this->data_.push_data(data.data(), data.size(), false);
      }

    private:
        binary_serializer data_;

        template<typename transaction_type>
        expected<void> add_transaction(expected<transaction_type> && item) {
          if (item.has_error()) {
            return unexpected(std::move(item.error()));
          }

          CHECK_EXPECTED(this->data_ << (uint8_t)transaction_type::message_id);
          _serialize_visitor v(this->data_);
          const_cast<transaction_type &>(item.value()).visit(v);

          if (v.error()) {
            return unexpected(std::move(v.error()));
          }

          return expected<void>();
        }
    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_
