#ifndef __VDS_DB_MODEL_SYNC_REPLICA_PAYMENT_DBO_H__
#define __VDS_DB_MODEL_SYNC_REPLICA_PAYMENT_DBO_H__
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <chrono>
#include "database_orm.h"

namespace vds {
	namespace orm {
		class sync_replica_payment_dbo : public database_table {
		public:
			sync_replica_payment_dbo()
				: database_table("chunk_replica_payment"),
				owner_id(this, "owner_id"),
				replica_hash(this, "replica_hash"),
				node(this, "node"),
				last_payment(this, "last_payment") {
			}

			database_column<const_data_buffer, std::string> owner_id;
			database_column<const_data_buffer, std::string> replica_hash;
			database_column<const_data_buffer, std::string> node;
			database_column<std::chrono::system_clock::time_point> last_payment;

			static constexpr const char * create_table =       
				"CREATE TABLE chunk_replica_payment(\
				owner_id VARCHAR(64) NOT NULL,\
				replica_hash VARCHAR(64) NOT NULL,\
				node VARCHAR(64) NOT NULL,\
				last_payment INTEGER NOT NULL,\
				CONSTRAINT pk_chunk_replica_payment PRIMARY KEY(owner_id,replica_hash,node))";
		};
	}
}


#endif//__VDS_DB_MODEL_SYNC_REPLICA_PAYMENT_DBO_H__