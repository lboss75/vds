#ifndef __VDS_DB_MODEL_CERTIFICATE_UNKNOWN_DBO_H_
#define __VDS_DB_MODEL_CERTIFICATE_UNKNOWN_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {

    class certificate_unknown_dbo : public database_table {
    public:
      certificate_unknown_dbo()
          : database_table("certificate_unknown"),
            id(this, "id") {
      }

      database_column<guid> id;
    };
  }
}

#endif //__VDS_DB_MODEL_CERTIFICATE_UNKNOWN_DBO_H_
