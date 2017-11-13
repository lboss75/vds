#ifndef __VDS_DB_MODEL_DEVICE_USER_DBO_H_
#define __VDS_DB_MODEL_DEVICE_USER_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {

  class device_user_dbo : public database_table {
  public:
    device_user_dbo()
        : database_table("device_user"),
          id(this, "id"),
          parent(this, "parent")
    {
    }

    database_column<guid> id;
    database_column<guid> parent;
  };
}

#endif //__VDS_DB_MODEL_DEVICE_USER_DBO_H_
