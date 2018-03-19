#ifndef __VDS_DB_MODEL_LOCAL_DATA_CACHE_DBO_H_
#define __VDS_DB_MODEL_LOCAL_DATA_CACHE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {

    class local_data_cache_dbo : public database_table {
    public:
      local_data_cache_dbo()
          : database_table("local_data_cache"),
            id(this, "id"),
            data(this, "data") {
      }

      database_column<std::string> id;
      database_column<const_data_buffer> data;
    };
  }
}

#endif //__VDS_DB_MODEL_LOCAL_DATA_CACHE_DBO_H_
