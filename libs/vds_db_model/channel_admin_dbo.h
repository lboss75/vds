#ifndef __VDS_DB_MODEL_CHANNEL_ADMIN_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_ADMIN_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {

  class channel_admin_dbo : public database_table {
  public:

    channel_admin_dbo()
        : database_table("channel_admin"),
          id(this, "id"),
          member_id(this, "member_id")
    {
    }

    database_column<guid> id;
    database_column<guid> member_id;
  };
}

#endif //__VDS_DB_MODEL_CHANNEL_ADMIN_DBO_H_
