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
      enum class state_t : int {
        none,
        processed,
        leaf
      };
      static std::string str(state_t value){
        switch(value){
          case state_t::none:
            return "none";

          case state_t::processed:
            return "processed";

          case state_t::leaf:
            return "leaf";

          default:
            return "unknown(" + std::to_string((int)value) + ")";
        }
      }

      transaction_log_record_dbo()
          : database_table("transaction_log_record"),
            id(this, "id"),
            channel_id(this, "channel_id"),
            data(this, "data"),
            state(this, "state"),
            order_no(this, "order_no") {
      }

      database_column<std::string> id;
      database_column<guid> channel_id;
      database_column<const_data_buffer> data;
      database_column<int> state;
      database_column<int> order_no;
    };
  }
}

#endif //__VDS_DB_MODEL_TRANSACTION_LOG_RECORD_H_
