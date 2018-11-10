//#ifndef __VDS_DB_MODEL_DATACOIN_BALANCE_DBO_H_
//#define __VDS_DB_MODEL_DATACOIN_BALANCE_DBO_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "database_orm.h"
//
//namespace vds {
//  namespace orm {
//    class datacoin_balance_dbo : public database_table {
//    public:
//      datacoin_balance_dbo()
//          : database_table("datacoin_balance"),
//            owner(this, "owner"),
//            balance(this, "balance") {
//      }
//
//      database_column<const_data_buffer, std::string> owner;
//      database_column<int64_t> balance;
//    };
//  }
//}
//
//#endif //__VDS_DB_MODEL_DATACOIN_BALANCE_DBO_H_
