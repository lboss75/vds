#ifndef __VDS_DB_MODEL_TRANSACTION_LOG_RECORD_H_
#define __VDS_DB_MODEL_TRANSACTION_LOG_RECORD_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class transaction_log_record_dbo : public database_table {
    public:
      enum class state_t : uint8_t {
        new_record = 0,
        pending,
        unknown,
        follower
      };

      transaction_log_record_dbo()
          : database_table("transaction_log_record"),
            id(this, "id"),
            state(this, "state") {}

      database_column<std::string> id;
      database_column<uint8_t> state;
    };
  }
}

#endif //__VDS_DB_MODEL_TRANSACTION_LOG_RECORD_H_
