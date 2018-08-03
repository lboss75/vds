#ifndef __VDS_DB_MODEL_TRANSACTION_LOG_UNKNOWN_RECORD_DBO_H_
#define __VDS_DB_MODEL_TRANSACTION_LOG_UNKNOWN_RECORD_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class transaction_log_unknown_record_dbo : public database_table {
    public:

      transaction_log_unknown_record_dbo()
          : database_table("transaction_log_unknown_record"),
            id(this, "id"),
            follower_id(this, "follower_id") {}

      database_column<const_data_buffer, std::string> id;
      database_column<const_data_buffer, std::string> follower_id;
    };
  }
}


#endif //__VDS_DB_MODEL_TRANSACTION_LOG_UNKNOWN_RECORD_DBO_H_
