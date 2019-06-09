#ifndef __VDS_DB_MODEL_DATACOIN_BALANCE_DBO_H_
#define __VDS_DB_MODEL_DATACOIN_BALANCE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class datacoin_balance_dbo : public database_table {
    public:
      datacoin_balance_dbo()
        : database_table("datacoin_balance"),
          owner(this, "owner"),
          issuer(this, "issuer"),
          currency(this, "currency"),
          source_transaction(this, "source_transaction"),
          confirmed_balance(this, "confirmed_balance"),
          proposed_balance(this, "proposed_balance"){
      }

      database_column<const_data_buffer, std::string> owner;
      database_column<const_data_buffer, std::string> issuer;
      database_column<std::string> currency;
      database_column<const_data_buffer> source_transaction;
      database_column<int64_t> confirmed_balance;
      database_column<int64_t> proposed_balance;
    };
  }
}

#endif //__VDS_DB_MODEL_DATACOIN_BALANCE_DBO_H_
