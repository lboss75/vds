#ifndef __VDS_DB_MODEL_CHUNK_DBO_H_
#define __VDS_DB_MODEL_CHUNK_DBO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
	namespace orm {
    //Chunk info
		class chunk_dbo : public database_table {
		public:
			chunk_dbo()
				: database_table("chunk"),
        object_hash(this, "object_hash"),
        object_size(this, "object_size"),
        last_sync(this, "last_sync")
			{
			}

			database_column<const_data_buffer, std::string> object_hash;
      database_column<uint64_t, long> object_size;
      database_column<std::chrono::system_clock::time_point> last_sync;
    };
	}
}

#endif //__VDS_DB_MODEL_CHUNK_DBO_H_
