//#ifndef __VDS_DB_MODEL_MEMBER_USER_DBO_H_
//#define __VDS_DB_MODEL_MEMBER_USER_DBO_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "database_orm.h"
//#include "const_data_buffer.h"
//
//namespace vds {
//  namespace orm {
//
//    class member_user_dbo : public database_table {
//    public:
//      member_user_dbo()
//          : database_table("member_user"),
//            id(this, "id"),
//            log_id(this, "log_id") {
//      }
//
//      database_column<std::string> id;
//      database_column<const_data_buffer, std::string> log_id;
//    };
//  }
//}
//
//#endif //__VDS_DB_MODEL_MEMBER_USER_DBO_H_
