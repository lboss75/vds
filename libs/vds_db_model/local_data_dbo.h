#ifndef __VDS_DB_MODEL_LOCAL_DATA_DBO_H_
#define __VDS_DB_MODEL_LOCAL_DATA_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>
#include "database_orm.h"

namespace vds {
  namespace orm {
    class local_data_dbo : public database_table {
    public:
      local_data_dbo()
          : database_table("local_data_dbo"),
            id(this, "object_id"),
            data(this, "data"),
            create_time(this, "create_time"),
            is_new(this, "is_new"){
      }

      database_column<const_data_buffer, std::string> id;
      database_column<const_data_buffer> data;
      database_column<std::chrono::system_clock::time_point> create_time;
      database_column<bool> is_new;
    };
  }
}

#endif //__VDS_DB_MODEL_LOCAL_DATA_DBO_H_
