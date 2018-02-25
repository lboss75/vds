#ifndef __VDS_DB_MODEL_RAFT_CHANNEL_DBO_H_
#define __VDS_DB_MODEL_RAFT_CHANNEL_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace dbo {
    class raft_channel_dbo : public database_table {
    public:
      raft_channel_dbo()
          : database_table("raft_channel"),
            channel_id(this, "channel_id"),
            left_base_thread_id(this, "left_base_thread_id"),
            right_base_thread_id(this, "right_base_thread_id") {
      }

      database_column<guid> channel_id;
      database_column<const_data_buffer> left_base_thread_id;
      database_column<const_data_buffer> right_base_thread_id;
    };
  }
}

#endif //__VDS_DB_MODEL_RAFT_CHANNEL_DBO_H_
