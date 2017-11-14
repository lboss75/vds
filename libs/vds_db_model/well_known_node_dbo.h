#ifndef __VDS_DB_MODEL_WELL_KNOWN_NODE_DBO_H_
#define __VDS_DB_MODEL_WELL_KNOWN_NODE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {

  class well_known_node_dbo : public database_table {
  public:
    well_known_node_dbo()
        : database_table("well_known_node"),
          id(this, "id"),
          addresses(this, "addresses")
    {
    }

    database_column<guid> id;
    database_column<std::string> addresses;
  };
}

#endif //__VDS_DB_MODEL_WELL_KNOWN_NODE_DBO_H_
