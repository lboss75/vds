#ifndef __VDS_DB_MODEL_RAFT_CHANNEL_MEMBER_DBO_H_
#define __VDS_DB_MODEL_RAFT_CHANNEL_MEMBER_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace dbo {
    class raft_channel_member_dbo : public database_table {
    public:
      raft_channel_member_dbo()
          : database_table("raft_channel_member"),
            channel_id(this, "channel_id"),
            device_id(this, "device_id"),
            is_client(this, "is_client"){
      }

      database_column<guid> channel_id;
      database_column<guid> device_id;
      database_column<bool> is_client;
    };
  }
}

#endif //__VDS_DB_MODEL_RAFT_CHANNEL_MEMBER_DBO_H_
