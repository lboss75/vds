/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifndef __VDS_TRANSACTIONS_TRANSACTION_STATE_CALCULATOR_H_
#define __VDS_TRANSACTIONS_TRANSACTION_STATE_CALCULATOR_H_

#include <database.h>
#include "include/transaction_record_state.h"

namespace vds {
  namespace transactions {
    class transaction_state_calculator {
    public:
      static transaction_record_state calculate(
          struct database_transaction &t,
          const std::set<const_data_buffer> & ancestors,
          uint64_t max_order_no);

    private:
      std::map<uint64_t, std::set<const_data_buffer>> not_processed_;
      std::map<uint64_t, std::map<const_data_buffer, const_data_buffer>> items_;

      void add_ancestor(
          vds::database_transaction &t,
          const const_data_buffer &ancestor_id,
          uint64_t max_order_no);

      transaction_record_state process(database_transaction &t);

      void resolve(
          database_transaction &t,
          const uint64_t order_no,
          const const_data_buffer &node_id);
    };
  }
}

#endif //__VDS_TRANSACTIONS_TRANSACTION_STATE_CALCULATOR_H_
