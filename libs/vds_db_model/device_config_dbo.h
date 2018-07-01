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
        reserved_size(this, "reserved_size"){
      }

      database_column<std::string> node_id;
      database_column<std::string> local_path;

      database_column<std::string> owner_id;

      database_column<std::string> name;
      database_column<uint64_t> reserved_size;
    };
  }
}

#endif //__VDS_DB_MODEL_DEVICE_CONFIG_DBO_H_
