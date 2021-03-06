#ifndef __VDS_DB_MODEL_CURRENT_CONFIG_DBO_H_
#define __VDS_DB_MODEL_CURRENT_CONFIG_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class current_config_dbo : public database_table {
    public:
      current_config_dbo()
          : database_table("current_config"),
            node_id(this, "node_id"),
            public_key(this, "public_key"),
            private_key(this, "private_key") {
      }

      database_column<const_data_buffer, std::string> node_id;

      database_column<const_data_buffer> public_key;
      database_column<const_data_buffer> private_key;

    };
  }
}

#endif //__VDS_DB_MODEL_CURRENT_CONFIG_DBO_H_
