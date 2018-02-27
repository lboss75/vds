#ifndef __VDS_DB_MODEL_TRANSACTION_LOG_LINK_DBO_H_
#define __VDS_DB_MODEL_TRANSACTION_LOG_LINK_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class transaction_log_link_dbo : public database_table {
    public:
      transaction_log_link_dbo()
          : database_table("transaction_log_link"),
            predecessor_id(this, "predecessor_id"),
            follower_id(this, "follower_id"),
            channel_id(this, "channel_id") {
      }

      database_column<std::string> predecessor_id;
      database_column<std::string> follower_id;
      database_column<guid> channel_id;
    };
  }
}

#endif //__VDS_DB_MODEL_TRANSACTION_LOG_LINK_DBO_H_
