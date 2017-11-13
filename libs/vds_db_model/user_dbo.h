#ifndef __VDS_DB_MODEL_USER_DBO_H_
#define __VDS_DB_MODEL_USER_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {

  class user_dbo : public database_table {
  public:
    user_dbo()
        : database_table("user"),
          id(this, "id"),
          private_key(this, "private_key"),
          parent(this, "parent"),
          login(this, "login"),
          password_hash(this, "password_hash")
    {
    }

    database_column<guid> id;
    database_column<const_data_buffer> private_key;
    database_column<guid> parent;

    database_column<std::string> login;
    database_column<std::string> password_hash;
  };
}

#endif //__VDS_DB_MODEL_USER_DBO_H_
