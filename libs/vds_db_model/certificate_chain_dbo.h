#ifndef __VDS_DB_MODEL_CERTIFICATE_CHAIN_DBO_H_
#define __VDS_DB_MODEL_CERTIFICATE_CHAIN_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace orm {

    class certificate_chain_dbo : public database_table {
    public:
		certificate_chain_dbo()
          : database_table("certificate_chain"),
            id(this, "id"),
            cert(this, "cert"),
            parent(this, "parent") {
      }

      database_column<std::string> id;
      database_column<const_data_buffer> cert;
      database_column<std::string> parent;
    };
  }
}

#endif //__VDS_DB_MODEL_CERTIFICATE_CHAIN_DBO_H_
