#ifndef __VDS_DB_MODEL_CURRENT_CONFIG_DBO_H_
#define __VDS_DB_MODEL_CURRENT_CONFIG_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class current_config_dbo : public database_table {
    public:
      current_config_dbo()
          : database_table("current_config"),
            node_id(this, "node_id"),
            local_path(this, "local_path"),
            owner_id(this, "owner_id"),
            reserved_size(this, "reserved_size"),
            public_key(this, "public_key"),
            private_key(this, "private_key") {
      }

      database_column<const_data_buffer, std::string> node_id;
      database_column<std::string> local_path;

      database_column<const_data_buffer, std::string> owner_id;

      database_column<int64_t> reserved_size;

      database_column<const_data_buffer> public_key;
      database_column<const_data_buffer> private_key;

      struct device_info {
        std::string local_path;
        int64_t reserved_size;
        int64_t used_size;
        int64_t free_size;

        device_info()
          : reserved_size(0), used_size(0), free_size(0) {
        }
      };

      static expected<device_info> get_free_space(
        database_read_transaction & t);
    };
  }
}

#endif //__VDS_DB_MODEL_CURRENT_CONFIG_DBO_H_
