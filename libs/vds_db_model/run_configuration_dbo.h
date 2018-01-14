#ifndef __VDS_DB_MODEL_RUN_CONFIGURATION_DBO_H_
#define __VDS_DB_MODEL_RUN_CONFIGURATION_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace dbo {
    class run_configuration : public database_table {
    public:
      run_configuration()
          : database_table("run_configuration"),
            id(this, "id"),
            cert_id(this, "cert_id"),
            cert_private_key(this, "cert_private_key"),
            port(this, "port"),
            common_channel_id(this, "common_channel_id") {
      }

      database_column <guid> id;
      database_column <guid> cert_id;
      database_column <const_data_buffer> cert_private_key;
      database_column<int> port;
      database_column <guid> common_channel_id;
    };
  }
}


#endif //__VDS_DB_MODEL_RUN_CONFIGURATION_DBO_H_
