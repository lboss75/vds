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

namespace vds {
  namespace transactions {
    class payment_transaction;

    class transaction_block_builder {
    public:

      transaction_block_builder(
        const service_provider &sp,
        class vds::database_transaction &t);

      static transaction_block_builder create_root_block() {
        return transaction_block_builder();
      }
      
      void add(const root_user_transaction & item);
      void add(const payment_transaction & item);

      template<typename item_type>
      void add(
        const const_data_buffer & channel_id,
        const certificate & write_cert,
        const asymmetric_private_key & write_key,
        const certificate & channel_read_cert,
        item_type && item) {

        auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

        binary_serializer s;
        s
          << (uint8_t)item_type::message_id;
        item.serialize(s);

        channel_message(
          channel_id,
          channel_read_cert.subject(),
          write_cert.subject(),
          channel_read_cert.public_key().encrypt(key.serialize()),
          symmetric_encrypt::encrypt(key, s.data().data(), s.data().size()),
          write_key)
        .serialize(this->data_);
      }

      const_data_buffer save(
          const service_provider &sp,
          class vds::database_transaction &t,
          const certificate &write_cert,
          const asymmetric_private_key &write_private_key);

      private:
      std::set<const_data_buffer> ancestors_;
      data_coin_balance balance_;
      binary_serializer data_;

      transaction_block_builder() {
      }


      const_data_buffer register_transaction(
		      const service_provider & sp,
          class database_transaction &t,
          const const_data_buffer & channel_id,
          const const_data_buffer &block,
          uint64_t order_no,
          const std::set<const_data_buffer> &ancestors) const;

      void on_new_transaction(
          const service_provider &sp,
          class database_transaction &t,
          const const_data_buffer & id,
          const const_data_buffer &block) const;
    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_
