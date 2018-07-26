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
        member_node(this, "member_node"),
        generation(this, "generation"),
        current_term(this, "current_term"),
        commit_index(this, "commit_index"),
        last_applied(this, "last_applied"),
        last_activity(this, "last_activity")
		  {
			}

			database_column<const_data_buffer, std::string> object_id;
      database_column<const_data_buffer, std::string> member_node;

      database_column<uint64_t> generation;
      database_column<uint64_t> current_term;
      database_column<uint64_t> commit_index;
      database_column<uint64_t> last_applied;

      database_column<std::chrono::system_clock::time_point> last_activity;
    };
	}
}

#endif //__VDS_DB_MODEL_SYNC_STATE_DBO_H_
