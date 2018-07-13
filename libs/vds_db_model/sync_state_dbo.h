#ifndef __VDS_DB_MODEL_SYNC_STATE_DBO_H_
#define __VDS_DB_MODEL_SYNC_STATE_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
		class sync_state_dbo : public database_table {
		public:
      enum class state_t : uint8_t {
        follower,
        canditate,
        leader
      };

			sync_state_dbo()
			: database_table("sync_state"),
				object_id(this, "object_id"),
        state(this, "state"),
        next_sync(this, "next_sync"),
        voted_for(this, "voted_for"),
        generation(this, "generation"),
        current_term(this, "current_term"),
        commit_index(this, "commit_index"),
        last_applied(this, "last_applied")
			{
			}

			database_column<std::string> object_id;
      database_column<uint8_t> state;
      database_column<std::chrono::system_clock::time_point> next_sync;
      database_column<std::string> voted_for;
      database_column<uint64_t> generation;
      database_column<uint64_t> current_term;
      database_column<uint64_t> commit_index;
      database_column<uint64_t> last_applied;

    };
	}
}

#endif //__VDS_DB_MODEL_SYNC_STATE_DBO_H_
