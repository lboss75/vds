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
            storage_id(this, "storage_id"),
            replica_hash(this, "replica_hash"),
            replica_size(this, "replica_size"),
            owner(this, "owner"),
            storage_path(this, "storage_path"),
            last_access(this, "last_access") {
      }

      database_column<const_data_buffer, std::string> storage_id;
      database_column<const_data_buffer, std::string> replica_hash;
      database_column<size_t, long> replica_size;
      database_column<const_data_buffer, std::string> owner;
      database_column<std::string> storage_path;
      database_column<std::chrono::system_clock::time_point> last_access;
    };
  }
}

#endif //__VDS_DB_MODEL_LOCAL_DATA_DBO_H_
