#ifndef __VDS_DB_MODEL_DEVICE_RECORD_DBO_H_
#define __VDS_DB_MODEL_DEVICE_RECORD_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class device_record_dbo : public database_table {
    public:
      device_record_dbo()
      : database_table("device_record"),
        node_id(this, "node_id"),
        storage_path(this, "storage_path"),
        local_path(this, "local_path"),
        data_hash(this, "data_hash"),
        data_size(this, "data_size"){
      }

      database_column<const_data_buffer, std::string> node_id;
      database_column<std::string> storage_path;

      database_column<std::string> local_path;

      database_column<const_data_buffer, std::string> data_hash;
      database_column<int64_t> data_size;
    };
  }
}

#endif //__VDS_DB_MODEL_DEVICE_RECORD_DBO_H_
