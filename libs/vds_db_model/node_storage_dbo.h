#ifndef __VDS_DB_MODEL_NODE_STORAGE_DBO_H_
#define __VDS_DB_MODEL_NODE_STORAGE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>
#include "database_orm.h"

namespace vds {
  namespace orm {
    class node_storage_dbo : public database_table {
    public:
      node_storage_dbo()
      : database_table("node_storage_dbo"),
        storage_id(this, "storage_id"),
        local_path(this, "local_path"),
        owner_id(this, "owner_id"),
        reserved_size(this, "reserved_size") {
      }

      database_column<const_data_buffer, std::string> storage_id;

      database_column<std::string> local_path;
      database_column<const_data_buffer, std::string> owner_id;
      database_column<int64_t> reserved_size;
    };
  }
}

#endif //__VDS_DB_MODEL_NODE_STORAGE_DBO_H_
