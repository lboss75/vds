#ifndef __VDS_DB_MODEL_CHUNK_DATA_DBO_H_
#define __VDS_DB_MODEL_CHUNK_DATA_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace dbo {

    class chunk_data_dbo : public database_table {
    public:
      chunk_data_dbo()
          : database_table("chunk_data"),
            id(this, "id"),
            block_key(this, "block_key"),
            block_data(this, "block_data") {
      }

      database_column<std::string> id;
      database_column<const_data_buffer> block_key;
      database_column<const_data_buffer> block_data;
    };
  }
}

#endif //__VDS_DB_MODEL_CHUNK_DATA_DBO_H_
