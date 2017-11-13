#ifndef __VDS_DB_MODEL_USER_CHANNEL_DBO_H_
#define __VDS_DB_MODEL_USER_CHANNEL_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {

  class user_channel_dbo : public database_table {
  public:
    user_channel_dbo()
        : database_table("user_channel"),
          id(this, "id"),
          owner(this, "owner")
    {
    }

    database_column<guid> id;
    database_column<guid> owner;
  };
}

#endif //__VDS_DB_MODEL_CHANNEL_DBO_H_
