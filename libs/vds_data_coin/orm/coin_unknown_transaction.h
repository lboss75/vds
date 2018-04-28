
#ifndef __VDS_DATA_COIN_COIN_UNKNOWN_TRANSACTION_H_
#define __VDS_DATA_COIN_COIN_UNKNOWN_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <database_orm.h>

namespace vds {
  namespace data_coin {
    namespace orm {
      class coin_unknown_transaction : public database_table {
      public:
        coin_unknown_transaction()
            : database_table("coin_unknown_transaction"),
              transaction_id(this, "transaction_id"){
        }

        database_column<std::string> transaction_id;
      };
    }
  }
}

#endif //__VDS_DATA_COIN_COIN_UNKNOWN_TRANSACTION_H_
