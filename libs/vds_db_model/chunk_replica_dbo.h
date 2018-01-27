#ifndef __VDS_DB_MODEL_CHUNK_REPLICA_DBO_H_
#define __VDS_DB_MODEL_CHUNK_REPLICA_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace dbo {
		class chunk_replica_dbo : public database_table {
		public:
			chunk_replica_dbo()
				: database_table("chunk_replica"),
				id(this, "id"),
				replica(this, "replica"),
				replica_data(this, "replica_data")
			{
			}

			database_column<std::string> id;
			database_column<int> replica;
			database_column<const_data_buffer> replica_data;
		};
	}
}

#endif //__VDS_DB_MODEL_CHUNK_REPLICA_DBO_H_
