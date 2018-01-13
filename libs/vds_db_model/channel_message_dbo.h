#ifndef __VDS_DB_MODEL_CHANNEL_MESSAGE_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_MESSAGE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class channel_message_dbo : public database_table {
    public:

      channel_message_dbo()
          : database_table("channel_message"),
            id(this, "id"),
            channel_id(this, "channel_id"),
            message_id(this, "message_id"),
            read_cert_id(this, "read_cert_id"),
            write_cert_id(this, "write_cert_id"),
            message(this, "message") {
      }

      database_column<int> id;
      database_column <guid> channel_id;
      database_column<int> message_id;
      database_column <guid> read_cert_id;
      database_column <guid> write_cert_id;
      database_column <const_data_buffer> message;
    };
  }
}

#endif //__VDS_DB_MODEL_CHANNEL_MESSAGE_DBO_H_
