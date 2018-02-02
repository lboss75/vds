#ifndef __VDS_DB_MODEL_CHUNK_REPLICA_DATA_DBO_H_
#define __VDS_DB_MODEL_CHUNK_REPLICA_DATA_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace dbo {
		class chunk_replica_data_dbo : public database_table {
		public:
			chunk_replica_data_dbo()
				: database_table("chunk_replica_data"),
				id(this, "id"),
				replica(this, "replica"),
        distance(this, "distance"),
        replica_data(this, "replica_data"),
        replica_hash(this, "replica_hash")
			{
			}

			database_column<std::string> id;
			database_column<int> replica;
      database_column<uint64_t> distance;
      database_column<const_data_buffer> replica_data;
      database_column<const_data_buffer> replica_hash;
		};
	}
}

#endif //__VDS_DB_MODEL_CHUNK_REPLICA_DATA_DBO_H_
