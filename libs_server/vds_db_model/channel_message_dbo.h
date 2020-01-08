#ifndef __VDS_DB_MODEL_CHANNEL_MESSAGE_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_MESSAGE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {

    class channel_message_dbo : public database_table {
    public:
      channel_message_dbo()
          : database_table("channel_message_dbo"),
            id(this, "id"),
        block_id(this, "block_id"),
        channel_id(this, "channel_id"),
        read_id(this, "read_id"),
        write_id(this, "write_id"),
        crypted_key(this, "crypted_key"),
        crypted_data(this, "crypted_data"),
        signature(this, "signature")
      {
      }

      database_column<int64_t> id;
      database_column<const_data_buffer, std::string> block_id;
      database_column<const_data_buffer, std::string> channel_id;
      database_column<const_data_buffer, std::string> read_id;
      database_column<const_data_buffer, std::string> write_id;
      database_column<const_data_buffer, std::string> crypted_key;
      database_column<const_data_buffer> crypted_data;
      database_column<const_data_buffer, std::string> signature;
    };
  }
}

#endif //__VDS_DB_MODEL_CHANNEL_MESSAGE_DBO_H_
