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

  class run_configuration_dbo : public database_table {
  public:
    run_configuration_dbo()
        : database_table("run_configuration"),
          id(this, "id"),
          cert_id(this, "cert_id"),
          port(this, "port")
    {
    }

    database_column<guid> id;
    database_column<guid> cert_id;
    database_column<int> port;
  };
}


#endif //__VDS_DB_MODEL_RUN_CONFIGURATION_DBO_H_
