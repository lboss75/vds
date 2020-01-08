//#ifndef __VDS_DB_MODEL_REGISTER_REQUEST_H_
//#define __VDS_DB_MODEL_REGISTER_REQUEST_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include <chrono>
//#include "database_orm.h"
//
//namespace vds {
//  namespace orm {
//    class register_request : public database_table {
//    public:
//      register_request()
//          : database_table("register_request"),
//            id(this, "id"),
//            name(this, "name"),
//            email(this, "email"),
//            data(this, "data"),
//            create_time(this, "create_time") {
//      }
//
//      database_column<const_data_buffer, std::string> id;
//      database_column<std::string> name;
//      database_column<std::string> email;
//      database_column<const_data_buffer> data;
//      database_column<std::chrono::system_clock::time_point> create_time;
//    };
//  }
//}
//
//#endif //__VDS_DB_MODEL_REGISTER_REQUEST_H_
