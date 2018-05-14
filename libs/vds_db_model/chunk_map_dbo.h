#ifndef __VDS_DB_MODEL_CHUNK_MAP_DBO_H_
#define __VDS_DB_MODEL_CHUNK_MAP_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
		class chunk_map_dbo : public database_table {
		public:
			chunk_map_dbo()
				: database_table("chunk_map"),
				  id(this, "id"),
					source_hash(this, "source_hash"),
					source_lenght(this, "source_lenght"),
					source_order(this, "source_order")
			{
			}

			database_column<int> id;
			database_column<std::string> source_hash;
      database_column<int> source_lenght;
			database_column<int> source_order;
		};
	}
}

#endif //__VDS_DB_MODEL_CHUNK_MAP_DBO_H_
