#ifndef __VDS_DB_MODEL_CHANNEL_LOCAL_CACHE_DBO_H_
#define __VDS_DB_MODEL_CHANNEL_LOCAL_CACHE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace orm {

    //class channel_local_cache_dbo : public database_table {
    //public:
    //  channel_local_cache_dbo()
    //      : database_table("channel_local_cache"),
    //        channel_id(this, "channel_id"),
    //        last_sync(this, "last_sync") {
    //  }

    //  database_column<std::string> channel_id;
    //  database_column<std::chrono::system_clock::time_point> last_sync;
    //};
  }
}

#endif //__VDS_DB_MODEL_CHANNEL_LOCAL_CACHE_DBO_H_
