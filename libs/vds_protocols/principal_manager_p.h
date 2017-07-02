#ifndef __VDS_PROTOCOLS_PRINCIPAL_MANAGER_P_H_
#define __VDS_PROTOCOLS_PRINCIPAL_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "principal_manager.h"
#include "database_orm.h"

namespace vds {
  class _principal_manager
  {
  public:
    
    static void create_database_objects(
      const service_provider & sp,
      uint64_t db_version,
      database_transaction & t);

  private:
    //Database
    class principal_table : public database_table
    {
    public:
      principal_table()
      : database_table("principal"),
        id(this, "id"),
        cert(this, "cert"),
        key(this, "key"),
        password_hash(this, "password_hash"),
        parent(this, "parent")
      {
      }
      
      database_column<guid> id;
      database_column<std::string> cert;
      database_column<std::string> key;
      database_column<const_data_buffer> password_hash;
      database_column<guid> parent;
    };
    
    void add_principal(
      const service_provider & sp,
      const principal_record & record);

    void add_user_principal(
      const service_provider & sp,
      const std::string & login,
      const principal_record & record);

    guid get_root_principal(
      const service_provider & sp);

    std::unique_ptr<principal_record> find_principal(
      const service_provider & sp,
      const guid & object_name);

    std::unique_ptr<principal_record> find_user_principal(
      const service_provider & sp,
      const std::string & object_name);
  };
}

#endif // __VDS_PROTOCOLS_PRINCIPAL_MANAGER_P_H_
