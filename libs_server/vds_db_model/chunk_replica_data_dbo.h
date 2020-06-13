#ifndef __VDS_DB_MODEL_CHUNK_REPLICA_DATA_DBO_H_
#define __VDS_DB_MODEL_CHUNK_REPLICA_DATA_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
		class chunk_replica_data_dbo : public database_table {
		public:
			chunk_replica_data_dbo()
				: database_table("chunk_replica_data"),
				owner_id(this, "owner_id"),
				object_hash(this, "object_hash"),
				replica(this, "replica"),
        replica_hash(this, "replica_hash"),
        replica_size(this, "replica_size"),
				distance(this, "distance") {
			}

			database_column<const_data_buffer, std::string> owner_id;
			database_column<const_data_buffer, std::string> object_hash;
			database_column<int16_t, int> replica;
      database_column<const_data_buffer, std::string> replica_hash;
      database_column<uint32_t, int> replica_size;
			database_column<uint32_t, int> distance;

			static constexpr const char* create_table =
				"CREATE TABLE chunk_replica_data(\
				owner_id  VARCHAR(64) NOT NULL,\
				object_hash VARCHAR(64) NOT NULL,\
				replica INTEGER NOT NULL,\
				replica_hash VARCHAR(64) NOT NULL,\
				replica_size INTEGER NOT NULL,\
				distance INTEGER NOT NULL,\
				CONSTRAINT pk_chunk_replica_data PRIMARY KEY(owner_id, object_hash,replica),\
				CONSTRAINT idx_chunk_replica_data_replica_hash UNIQUE(replica_hash))";
		};
	}
}

#endif //__VDS_DB_MODEL_CHUNK_REPLICA_DATA_DBO_H_
