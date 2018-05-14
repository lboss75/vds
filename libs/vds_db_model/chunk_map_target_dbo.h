#ifndef __VDS_DB_MODEL_CHUNK_MAP_TARGET_DBO_H_
#define __VDS_DB_MODEL_CHUNK_MAP_TARGET_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
		class chunk_map_target_dbo : public database_table {
		public:
			chunk_map_target_dbo()
				: database_table("chunk_map_target"),
				id(this, "id"),
				replica(this, "replica"),
				node(this, "node")
			{
			}

			database_column<int> id;
			database_column<const_data_buffer> target_hash;
      database_column<int> replica;
		};
	}
}

#endif //__VDS_DB_MODEL_CHUNK_MAP_TARGET_DBO_H_
