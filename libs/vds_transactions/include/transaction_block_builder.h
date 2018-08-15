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
  class _user_channel;

  namespace transactions {
    class create_user_transaction;
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
      void add(const create_user_transaction & item);
      void add(const payment_transaction & item);
      void add(const channel_message & item);

      const_data_buffer save(
          const service_provider &sp,
          class vds::database_transaction &t,
          const certificate &write_cert,
          const asymmetric_private_key &write_private_key);

      private:
        friend class _user_channel;
        std::chrono::system_clock::time_point time_point_;

        std::set<const_data_buffer> ancestors_;
        data_coin_balance balance_;
        binary_serializer data_;

        transaction_block_builder();

    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_
