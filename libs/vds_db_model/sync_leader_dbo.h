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
				replica(this, "replica"),
        leader(this, "leader"),
        generation(this, "generation")
			{
			}

			database_column<std::string> object_id;
			database_column<int> replica;
      database_column<std::string> leader;
      database_column<int> generation;
		};
	}
}

#endif //__VDS_DB_MODEL_SYNC_LEADER_DBO_H_
