#ifndef __VDS_DB_MODEL_RAFT_CHANNEL_RECORD_DBO_H_
#define __VDS_DB_MODEL_RAFT_CHANNEL_RECORD_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace dbo {
    class raft_channel_record_dbo : public database_table {
    public:
      raft_channel_record_dbo()
          : database_table("raft_channel_record"),
            channel_id(this, "channel_id"),
            record_index(this, "record_index"),
            record_data(this, "record_data") {
      }

      database_column<guid> channel_id;
      database_column<uint64_t> record_index;
      database_column<const_data_buffer> record_data;
    };
  }
}

#endif //__VDS_DB_MODEL_RAFT_CHANNEL_RECORD_DBO_H_
