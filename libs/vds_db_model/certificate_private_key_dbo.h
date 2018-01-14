#ifndef __VDS_DB_MODEL_CERTIFICATE_PRIVATE_KEY_DBO_H_
#define __VDS_DB_MODEL_CERTIFICATE_PRIVATE_KEY_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace dbo {
    class certificate_private_key : public database_table {
    public:
      certificate_private_key()
          : database_table("cert_private_key"),
            id(this, "id"),
            owner_id(this, "owner_id"),
            body(this, "body") {
      }

      database_column <guid> id;
      database_column <guid> owner_id;

      /*
       * body = cert(owner_id).public_key().encrypt(private_key)
       *
       */
      database_column <const_data_buffer> body;
    };
  }
}

#endif //__VDS_DB_MODEL_CERTIFICATE_PRIVATE_KEY_DBO_H_
