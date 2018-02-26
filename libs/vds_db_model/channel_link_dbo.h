#ifndef __VDS_DB_MODEL_CHANNEL_LINK_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_LINK_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {
    class channel_link_dbo : public database_table {
    public:
      channel_link_dbo()
          : database_table("channel_link"),
            ancestor_id(this, "ancestor_id"),
            descendant_id(this, "descendant_id"){
      }

      database_column <guid> ancestor_id;
      database_column <guid> descendant_id;
    };
  }
}


#endif //__VDS_DB_MODEL_CHANNEL_DBO_H_
