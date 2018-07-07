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
            id(this, "object_id"),
            cert(this, "cert"),
            cert_key(this, "cert_key") {
      }

      database_column<int> id;
      database_column<const_data_buffer> cert;
      database_column<const_data_buffer> cert_key;
    };
  }
}

#endif //__VDS_DB_MODEL_CURRENT_CONFIG_DBO_H_
