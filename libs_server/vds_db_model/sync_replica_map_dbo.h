#ifndef __VDS_DB_MODEL_CHUNK_REPLICA_MAP_DBO_H_
#define __VDS_DB_MODEL_CHUNK_REPLICA_MAP_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <chrono>
#include "database_orm.h"

namespace vds {
	namespace orm {
		class sync_replica_map_dbo : public database_table {
		public:
			sync_replica_map_dbo()
				: database_table("chunk_replica_map"),
        replica_hash(this, "replica_hash"),
				node(this, "node"),
        last_access(this, "last_access") {
			}

			database_column<const_data_buffer, std::string> replica_hash;
      database_column<const_data_buffer, std::string> node;
      database_column<std::chrono::system_clock::time_point> last_access;

			static constexpr const char* create_table =
				"CREATE TABLE chunk_replica_map(\
				replica_hash VARCHAR(64) NOT NULL,\
				node VARCHAR(64) NOT NULL,\
				last_access INTEGER NOT NULL,\
				CONSTRAINT pk_chunk_replica_map PRIMARY KEY(replica_hash,node))";
		};
	}
}

#endif //__VDS_DB_MODEL_CHUNK_REPLICA_MAP_DBO_H_
