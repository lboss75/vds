#ifndef __VDS_DB_MODEL_TRANSACTION_LOG_RECORD_H_
#define __VDS_DB_MODEL_TRANSACTION_LOG_RECORD_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class transaction_log_record_dbo : public database_table {
    public:
      enum class state_t : uint8_t {
        validated,
        processed,
        leaf,
        invalid
      };

      static bool have_state(state_t value) {
        return value == state_t::leaf || value == state_t::processed;
      }

      static std::string str(state_t value){
        switch(value){
       
        case state_t::validated:
          return "validated";

        case state_t::processed:
            return "processed";

          case state_t::leaf:
            return "leaf";

          case state_t::invalid:
            return "invalid";

          default:
            return "unknown(" + std::to_string((uint8_t)value) + ")";
        }
      }

      transaction_log_record_dbo()
          : database_table("transaction_log_record"),
            id(this, "id"),
            data(this, "data"),
            state(this, "state"),
            consensus(this, "consensus"),
            new_member(this, "new_member"),
            order_no(this, "order_no"),
            time_point(this, "time_point"){
      }

      database_column<const_data_buffer, std::string> id;
      database_column<const_data_buffer> data;
      database_column<state_t, int> state;
      database_column<bool, int> consensus;
      database_column<bool, int> new_member;
      database_column<int64_t> order_no;
      database_column<std::chrono::system_clock::time_point> time_point;
    };
  }
}

#endif //__VDS_DB_MODEL_TRANSACTION_LOG_RECORD_H_
