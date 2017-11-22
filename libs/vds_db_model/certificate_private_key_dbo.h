#ifndef __VDS_DB_MODEL_CERTIFICATE_PRIVATE_KEY_DBO_H_
#define __VDS_DB_MODEL_CERTIFICATE_PRIVATE_KEY_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {

  class certificate_private_key_dbo : public database_table {
  public:
    certificate_private_key_dbo()
        : database_table("cert_private_key"),
          id(this, "id"),
          body(this, "body")
    {
    }

    database_column<guid> id;
    database_column<const_data_buffer> body;
  };
}

#endif //__VDS_DB_MODEL_CERTIFICATE_PRIVATE_KEY_DBO_H_
