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
      enum class usage_type_t {
        share,
        exclusive
      };

      node_storage_dbo()
      : database_table("node_storage_dbo"),
        storage_id(this, "storage_id"),
        local_path(this, "local_path"),
        owner_id(this, "owner_id"),
        reserved_size(this, "reserved_size"),
        usage_type(this, "usage_type") {
      }

      database_column<const_data_buffer, std::string> storage_id;

      database_column<std::string> local_path;
      database_column<const_data_buffer, std::string> owner_id;
      database_column<int64_t> reserved_size;
      database_column<usage_type_t, int32_t> usage_type;

      static expected<orm::node_storage_dbo::usage_type_t> parse_usage_type(const std::string& value) {
        if ("share" == value) {
          return orm::node_storage_dbo::usage_type_t::share;
        }
        else if ("exclusive" == value) {
          return orm::node_storage_dbo::usage_type_t::exclusive;
        }
        else {
          return make_unexpected<std::runtime_error>("Invalid storage usage type " + value);
        }
      }
    };
  }
}

namespace std {
  inline std::string to_string(vds::orm::node_storage_dbo::usage_type_t value) {
    switch (value) {
    case vds::orm::node_storage_dbo::usage_type_t::share:
      return "share";
    case vds::orm::node_storage_dbo::usage_type_t::exclusive:
      return "exclusive";
    default:
      return "unknown";
    }
  }
}


#endif //__VDS_DB_MODEL_NODE_STORAGE_DBO_H_
