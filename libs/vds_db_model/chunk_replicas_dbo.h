#ifndef __VDS_DB_MODEL_CHUNK_REPLICAS_DBO_H_
#define __VDS_DB_MODEL_CHUNK_REPLICAS_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
		class chunk_replicas_dbo : public database_table {
		public:
			chunk_replicas_dbo()
				: database_table("chunk_replicas"),
				id(this, "id"),
				replica_data(this, "replica_data"),
        last_sync(this, "last_sync")
			{
			}

			database_column<std::string> id;
			database_column<const_data_buffer> replica_data;
      database_column<std::chrono::system_clock::time_point> last_sync;
    };
	}
}

#endif //__VDS_DB_MODEL_CHUNK_REPLICAS_DBO_H_
