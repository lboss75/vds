#ifndef __VDS_DB_MODEL_CHUNK_MAP_DBO_H_
#define __VDS_DB_MODEL_CHUNK_MAP_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  class chunk_map_dbo : public database_table {
  public:
    chunk_map_dbo()
        : database_table("chunk_map"),
          id(this, "id"),
          replica(this, "replica"),
          device(this, "device")
    {
    }

    database_column<std::string> id;
    database_column<int> replica;
    database_column<guid> device;
  };

}

#endif //__VDS_DB_MODEL_CHUNK_MAP_DBO_H_
