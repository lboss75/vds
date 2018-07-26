#ifndef __VDS_DB_MODEL_SYNC_MESSAGE_DBO_H_
#define __VDS_DB_MODEL_SYNC_MESSAGE_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
		class sync_message_dbo : public database_table {
		public:
      enum class message_type_t : uint8_t {
        add_member,
        remove_member,
        add_replica,
        remove_replica
      };

			sync_message_dbo()
			: database_table("sync_message"),
				object_id(this, "object_id"),
        generation(this, "generation"),
        current_term(this, "current_term"),
        index(this, "index"),
        message_type(this, "message_type"),
        member_node(this, "member_node"),
				replica(this, "replica")
			{
			}

			database_column<const_data_buffer, std::string> object_id;
      database_column<uint64_t> generation;
      database_column<uint64_t> current_term;
      database_column<uint64_t> index;
      database_column<message_type_t, uint8_t> message_type;
      database_column<const_data_buffer, std::string> member_node;
			database_column<uint16_t> replica;

    };
	}
}

#endif //__VDS_DB_MODEL_SYNC_MESSAGE_DBO_H_
