#ifndef __VDS_DB_MODEL_CERTIFICATE_DBO_H_
#define __VDS_DB_MODEL_CERTIFICATE_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  namespace dbo {
    class certificate : public database_table {
    public:
      certificate()
          : database_table("cert"),
            id(this, "id"),
            cert(this, "body"),
            parent(this, "parent") {
      }

      database_column <guid> id;
      database_column <const_data_buffer> cert;
      database_column <guid> parent;
    };
  }
}

#endif //__VDS_DB_MODEL_CERTIFICATE_DBO_H_
