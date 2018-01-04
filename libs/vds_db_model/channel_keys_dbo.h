#ifndef __VDS_DB_MODEL_CHANNEL_KEYS_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_KEYS_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {

  class channel_keys_dbo : public database_table {
  public:

    channel_keys_dbo()
        : database_table("channel_keys"),
          channel_id(this, "channel_id"),
          user_id(this, "user_id"),
          cert_id(this, "cert_id"),
          read_key(this, "read_key"),
          write_key(this, "write_key")
    {
    }

    database_column<guid> channel_id;
    database_column<guid> user_id;
    database_column<guid> cert_id;
    database_column<const_data_buffer> read_key;
    database_column<const_data_buffer> write_key;
  };
}

#endif //__VDS_DB_MODEL_CHANNEL_KEYS_DBO_H_
