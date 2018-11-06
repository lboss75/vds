#ifndef __VDS_DB_MODEL_SYNC_LOCAL_QUEUE_DBO_H_
#define __VDS_DB_MODEL_SYNC_LOCAL_QUEUE_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "sync_message_dbo.h"

namespace vds {
	namespace orm {
		class sync_local_queue_dbo : public database_table {
		public:

			sync_local_queue_dbo()
			: database_table("sync_local_queue"),
        local_index(this, "local_index"),
        object_id(this, "object_id"),
        message_type(this, "message_type"),
        member_node(this, "member_node"),
				replica(this, "replica"),
        last_send(this, "last_send")
			{
			}

      database_column<int64_t, int> local_index;
      database_column<const_data_buffer, std::string> object_id;
      database_column<sync_message_dbo::message_type_t, int> message_type;
      database_column<const_data_buffer, std::string> member_node;
      database_column<int16_t, int> replica;
      database_column<std::chrono::system_clock::time_point> last_send;
    };
	}
}

#endif //__VDS_DB_MODEL_SYNC_LOCAL_QUEUE_DBO_H_
