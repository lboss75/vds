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
				id(this, "id"),
				replica(this, "replica"),
        replica_hash(this, "replica_hash"),
				replica_path(this, "replica_path")
			{
			}

			database_column<std::string> id;
			database_column<int> replica;
      database_column<std::string> replica_hash;
			database_column<std::string> replica_path;
		};
	}
}

#endif //__VDS_DB_MODEL_CHUNK_REPLICA_DATA_DBO_H_
