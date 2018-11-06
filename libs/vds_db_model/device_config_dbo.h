#ifndef __VDS_DB_MODEL_DEVICE_CONFIG_DBO_H_
#define __VDS_DB_MODEL_DEVICE_CONFIG_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class device_config_dbo : public database_table {
    public:
      device_config_dbo()
      : database_table("device_config"),
        node_id(this, "node_id"),
        local_path(this, "local_path"),
        owner_id(this, "owner_id"),
        name(this, "name"),
        reserved_size(this, "reserved_size"),
        cert(this, "cert"),
        private_key(this, "private_key") {
      }

      database_column<const_data_buffer, std::string> node_id;
      database_column<std::string> local_path;

      database_column<std::string> owner_id;

      database_column<std::string> name;
      database_column<int64_t> reserved_size;

      database_column<const_data_buffer, std::string> cert;
      database_column<const_data_buffer, std::string> private_key;

      struct device_info {
        std::string name;
        std::string local_path;
        uint64_t reserved_size;
        uint64_t used_size;
        uint64_t free_size;
      };

      static std::list<device_info> get_free_space(
        database_read_transaction & t,
        const const_data_buffer & node_id);
    };
  }
}

#endif //__VDS_DB_MODEL_DEVICE_CONFIG_DBO_H_
