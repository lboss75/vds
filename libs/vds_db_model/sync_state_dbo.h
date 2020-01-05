//#ifndef __VDS_DB_MODEL_SYNC_STATE_DBO_H_
//#define __VDS_DB_MODEL_SYNC_STATE_DBO_H_
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "database_orm.h"
//
//namespace vds {
//	namespace orm {
//		class sync_state_dbo : public database_table {
//		public:
//      enum class state_t : uint8_t {
//        follower,
//        canditate,
//        leader
//      };
//
//			sync_state_dbo()
//			: database_table("sync_state"),
//				object_id(this, "object_id"),
//        object_size(this, "object_size"),
//        state(this, "state"),
//        next_sync(this, "next_sync") {
//			}
//
//			database_column<const_data_buffer, std::string> object_id;
//      database_column<int32_t, int> object_size;
//      database_column<state_t, int> state;
//      database_column<std::chrono::system_clock::time_point> next_sync;
//    };
//	}
//}
//
//#endif //__VDS_DB_MODEL_SYNC_STATE_DBO_H_
