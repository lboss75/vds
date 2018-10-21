#ifndef __VDS_DB_MODEL_DATACOIN_DBO_H_
#define __VDS_DB_MODEL_DATACOIN_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>
#include "database_orm.h"

namespace vds {
  namespace orm {
    class datacoin_dbo : public database_table {
    public:
      datacoin_dbo()
          : database_table("datacoin_dbo"),
            id(this, "object_id"),
            data(this, "data"),
            create_time(this, "create_time"),
            is_new(this, "is_new"){
      }

      database_column<const_data_buffer, std::string> wallet_fingerprint;
      database_column<const_data_buffer> data;
      database_column<std::chrono::system_clock::time_point> create_time;
      database_column<bool> is_new;
    };
  }
}

#endif //__VDS_DB_MODEL_DATACOIN_DBO_H_
