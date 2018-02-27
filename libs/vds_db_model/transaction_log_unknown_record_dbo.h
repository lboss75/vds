#ifndef __VDS_DB_MODEL_TRANSACTION_LOG_UNKNOWN_RECORD_DBO_H_
#define __VDS_DB_MODEL_TRANSACTION_LOG_UNKNOWN_RECORD_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class transaction_log_unknown_record_dbo : public database_table {
    public:
      enum class relation_type_t : uint8_t {
        hard = 0,
        week
      };

      transaction_log_unknown_record_dbo()
          : database_table("transaction_log_unknown_record"),
            id(this, "id"),
            follower_id(this, "follower_id") {}

      database_column<guid> id;
      database_column<guid> channel_id;
      database_column<std::string> follower_id;
    };
  }
}


#endif //__VDS_DB_MODEL_TRANSACTION_LOG_UNKNOWN_RECORD_DBO_H_
