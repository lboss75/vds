#ifndef __VDS_DB_MODEL_CHANNEL_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {

  class channel_dbo : public database_table {
  public:
    channel_dbo()
        : database_table("channel"),
          id(this, "id"),
          cert(this, "cert")
    {
    }

    database_column<guid> id;
    database_column<guid> cert;
  };
}

#endif //__VDS_DB_MODEL_CHANNEL_DBO_H_
