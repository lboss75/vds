#ifndef __VDS_DB_MODEL_CHUNK_DATA_DBO_H_
#define __VDS_DB_MODEL_CHUNK_DATA_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class chunk_data_dbo : public database_table {
    public:
      chunk_data_dbo()
          : database_table("chunk_data"),
            id(this, "id"),
            data(this, "data"),
            last_access(this, "last_access"){
      }

      database_column<std::string> id;
      database_column<const_data_buffer> data;
      database_column<std::chrono::system_clock::time_point> last_access;
    };
  }
}

#endif //__VDS_DB_MODEL_CHUNK_DATA_DBO_H_
