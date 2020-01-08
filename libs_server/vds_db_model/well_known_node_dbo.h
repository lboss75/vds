#ifndef __VDS_DB_MODEL_WELL_KNOWN_NODE_DBO_H_
#define __VDS_DB_MODEL_WELL_KNOWN_NODE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class well_known_node_dbo : public database_table {
    public:
      well_known_node_dbo()
          : database_table("well_known_node"),
            address(this, "address"),
            last_connect(this, "last_connect") {
      }

      database_column<std::string> address;
      database_column<std::chrono::system_clock::time_point> last_connect;
    };
  }
}

#endif //__VDS_DB_MODEL_WELL_KNOWN_NODE_DBO_H_
