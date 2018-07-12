#ifndef __VDS_DB_MODEL_SYNC_LEADER_DBO_H_
#define __VDS_DB_MODEL_SYNC_LEADER_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
		class sync_leader_dbo : public database_table {
		public:
			sync_leader_dbo()
				: database_table("sync_leader"),
				object_id(this, "object_id"),
        leader(this, "leader"),
        last_sync(this, "last_sync")
			{
			}

			database_column<std::string> object_id;
      database_column<std::string> leader;
      database_column<std::chrono::system_clock::time_point> last_sync;
		};
	}
}

#endif //__VDS_DB_MODEL_SYNC_LEADER_DBO_H_
