#ifndef __VDS_DB_MODEL_CHANNEL_RECORD_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_RECORD_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class channel_record_dbo : public database_table {
    public:
      channel_record_dbo()
          : database_table("channel_record"),
            id(this, "id"),
            channel_id(this, "channel_id"),
            body(this, "body"){
      }

      database_column <guid> id;
      database_column <guid> channel_id;
      database_column <const_data_buffer> body;
    };
  }
}


#endif //__VDS_DB_MODEL_CHANNEL_RECORD_DBO_H_
