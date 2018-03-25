#ifndef __VDS_DB_MODEL_KNOWN_NODE_DBO_H_
#define __VDS_DB_MODEL_KNOWN_NODE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class known_node_dbo : public database_table {
    public:
      known_node_dbo()
          : database_table("known_node"),
            address(this, "address"),
            blocked(this, "blocked") {
      }

      database_column<std::string> address;
      database_column<bool> blocked;
    };
  }
}

#endif //__VDS_DB_MODEL_KNOWN_NODE_DBO_H_
