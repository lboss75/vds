#ifndef __VDS_DB_MODEL_CHANNEL_MEMBER_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_MEMBER_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {
    class channel_member_dbo : public database_table {
    public:

      enum class member_type_t : uint8_t {
        reader = 1,
        writer = 2,
        reader_and_writer = 3
      };

      channel_member_dbo()
          : database_table("channel_member"),
            channel_id(this, "channel_id"),
            user_cert_id(this, "user_cert_id"),
            member_type(this, "member_type") {
      }

      database_column <guid> channel_id;
      database_column <guid> user_cert_id;
      database_column <uint8_t> member_type;
    };
  }
}

#endif //__VDS_DB_MODEL_CHANNEL_MEMBER_DBO_H_
