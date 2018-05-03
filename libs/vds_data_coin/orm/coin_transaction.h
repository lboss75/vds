#ifndef __VDS_DATA_COIN_COIN_TRANSACTION_H_
#define __VDS_DATA_COIN_COIN_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <database_orm.h>

namespace vds {
  namespace data_coin {
    namespace orm {
      class coin_transaction : public database_table {
      public:
        enum class state_t {
          stored,
          validated,
          applied,

        };

        coin_transaction()
            : database_table("coin_transaction"),
              id(this, "id"),
              state(this, "state"),
              data(this, "data"),
              balance(this, "balance"),
              order_no(this, "order_no") {
        }

        database_column<std::string> id;
        database_column<int> state;
        database_column<const_data_buffer> data;
        database_column<const_data_buffer> balance;
        database_column<uint64_t> order_no;
      };
    }
  }
}

#endif //__VDS_DATA_COIN_COIN_TRANSACTION_H_
