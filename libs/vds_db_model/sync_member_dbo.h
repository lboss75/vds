#ifndef __VDS_DB_MODEL_SYNC_MEMBER_DBO_H_
#define __VDS_DB_MODEL_SYNC_MEMBER_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
		class sync_member_dbo : public database_table {
		public:
      sync_member_dbo()
				: database_table("sync_member"),
				object_id(this, "object_id"),
        member_id(this, "member_id"),
        last_activity(this, "last_activity")
			{
			}

			database_column<std::string> object_id;
      database_column<std::string> member_id;
      database_column<std::chrono::system_clock::time_point> last_activity;
    };
	}
}

#endif //__VDS_DB_MODEL_SYNC_STATE_DBO_H_
