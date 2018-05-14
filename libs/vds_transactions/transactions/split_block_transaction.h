#ifndef __VDS_TRANSACTIONS_SPLIT_BLOCK_TRANSACTION_H_
#define __VDS_TRANSACTIONS_SPLIT_BLOCK_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include "const_data_buffer.h"
#include "transaction_id.h"

namespace vds {
  namespace transactions {
    class split_block_transaction {
    public:
      static const transaction_id message_id = transaction_id::split_block_transaction;


    private:
      std::list<const_data_buffer> source_hash_;
      std::vector<const_data_buffer> target_hash_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_SPLIT_BLOCK_TRANSACTION_H_
