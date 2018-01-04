#ifndef __VDS_DB_MODEL_TRANSACTION_LOG_RECORD_RELATION_H_
#define __VDS_DB_MODEL_TRANSACTION_LOG_RECORD_RELATION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class transaction_log_record_relation_dbo : public database_table {
    public:
      enum class relation_type_t : uint8_t {
        hard = 0,
        week
      };

      transaction_log_record_relation_dbo()
          : database_table("transaction_log_record_relation"),
            predecessor_id(this, "predecessor_id"),
            follower_id(this, "follower_id"),
            relation_type(this, "relation_type") {}

      database_column<std::string> predecessor_id;
      database_column<std::string> follower_id;
      database_column<uint8_t> relation_type;
    };
  }
}

#endif //__VDS_DB_MODEL_TRANSACTION_LOG_RECORD_RELATION_H_
