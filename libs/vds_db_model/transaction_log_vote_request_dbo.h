#ifndef __VDS_DB_MODEL_TRANSACTION_LOG_VOTE_REQUEST_H_
#define __VDS_DB_MODEL_TRANSACTION_LOG_VOTE_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class transaction_log_vote_request_dbo : public database_table {
    public:
      transaction_log_vote_request_dbo()
          : database_table("transaction_log_vote_request"),
            id(this, "id"),
            owner(this, "owner"),
            approved(this, "approved") {
      }

      database_column<const_data_buffer, std::string> id;
      database_column<std::string> owner;
      database_column<bool, int> approved;
    };
  }
}

#endif //__VDS_DB_MODEL_TRANSACTION_LOG_VOTE_REQUEST_H_
