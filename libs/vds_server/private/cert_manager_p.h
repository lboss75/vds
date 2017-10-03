#ifndef __VDS_SERVER_CERT_MANAGER_P_H_
#define __VDS_SERVER_CERT_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "cert_manager.h"
#include "database_orm.h"

namespace vds {
  class _cert_manager : public cert_manager
  {
  public:
    bool validate(const certificate & cert);
    
    static void create_database_objects(
      const service_provider & sp,
      uint64_t db_version,
      database_transaction & t);

  private:
    class cert_table : public database_table
    {
    public:
      cert_table()
      : database_table("cert"),
        id(this, "id"),
        cert(this, "body"),
        parent(this, "parent")
      {
      }
      
      database_column<guid> id;
      database_column<const_data_buffer> cert;
      database_column<guid> parent;
    };

  };
}

#endif // __VDS_SERVER_CERT_MANAGER_P_H_
