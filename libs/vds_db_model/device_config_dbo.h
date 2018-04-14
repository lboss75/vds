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
            id(this, "id"),
            owner_id(this, "owner_id"),
            name(this, "name"),
            reserved_size(this, "reserved_size"),
            free_size(this, "free_size"){
      }

      database_column<std::string> id;
      database_column<std::string> owner_id;

      database_column<std::string> name;
      database_column<uint64_t> reserved_size;
      database_column<uint64_t> free_size;
    };
  }
}

#endif //__VDS_DB_MODEL_DEVICE_CONFIG_DBO_H_
