#ifndef __VDS_DB_MODEL_TRANSACTION_LOG_DEPENDENCY_H_
#define __VDS_DB_MODEL_TRANSACTION_LOG_DEPENDENCY_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class transaction_log_dependency_dbo : public database_table {
    public:
      transaction_log_dependency_dbo()
          : database_table("transaction_log_dependency"),
            channel_id(this, "channel_id"),
            block_id(this, "block_id"),
            block_key(this, "block_key") {}

      database_column<guid> channel_id;
      database_column<const_data_buffer> block_id;
      database_column<const_data_buffer> block_key;
    };
  }
}

#endif //__VDS_DB_MODEL_TRANSACTION_LOG_DEPENDENCY_H_
