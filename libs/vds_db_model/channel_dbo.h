#ifndef __VDS_DB_MODEL_CHANNEL_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class channel_dbo : public database_table {
    public:
      channel_dbo()
          : database_table("channel"),
            id(this, "id") {
      }

      database_column <guid> id;
    };
  }
}


#endif //__VDS_DB_MODEL_CHANNEL_DBO_H_
