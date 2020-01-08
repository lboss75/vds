#ifndef __VDS_DB_MODEL_WALLET_DBO_H_
#define __VDS_DB_MODEL_WALLET_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class wallet_dbo : public database_table {
    public:
      wallet_dbo()
          : database_table("wallet_dbo"),
        id(this, "id"),
        public_key(this, "public_key") {
      }

      database_column<const_data_buffer, std::string> id;
      database_column<const_data_buffer> public_key;
    };
  }
}

#endif //__VDS_DB_MODEL_WALLET_DBO_H_
