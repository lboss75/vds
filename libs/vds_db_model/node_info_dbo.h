#ifndef __VDS_DB_MODEL_NODE_INFO_DBO_H_
#define __VDS_DB_MODEL_NODE_INFO_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>
#include "database_orm.h"

namespace vds {
  namespace orm {
    class node_info_dbo : public database_table {
    public:
      node_info_dbo()
          : database_table("node_info_dbo"),
            node_id(this, "node_id"),
            public_key(this, "public_key") {
      }

      database_column<const_data_buffer, std::string> node_id;
      database_column<const_data_buffer, std::string> public_key;
    };
  }
}

#endif //__VDS_DB_MODEL_NODE_INFO_DBO_H_
