//#ifndef __VDS_DB_MODEL_USER_WALLET_DBO_H_
//#define __VDS_DB_MODEL_USER_WALLET_DBO_H_
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
//    class user_wallet_dbo : public database_table {
//    public:
//      user_wallet_dbo()
//          : database_table("user_wallet_dbo"),
//        owner(this, "owner"),
//        fingerprint(this, "fingerprint"),
//        name(this, "name"),
//        cert(this, "cert_private_key"),
//        cert_private_key(this, "cert_private_key") {
//      }
//
//      database_column<std::string> owner;
//      database_column<const_data_buffer, std::string> fingerprint;
//
//      database_column<std::string> name;
//      database_column<const_data_buffer> cert;
//      database_column<const_data_buffer> cert_private_key;
//    };
//  }
//}
//
//#endif //__VDS_DB_MODEL_USER_WALLET_DBO_H_
