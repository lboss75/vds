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

    enum class channel_type_t : uint8_t {
      simple = 0
    };

    channel_dbo()
        : database_table("channel"),
          id(this, "id"),
          channel_type(this, "channel_type")
    {
    }

    database_column<guid> id;
    database_column<uint8_t> channel_type;
  };
}

#endif //__VDS_DB_MODEL_CHANNEL_DBO_H_
