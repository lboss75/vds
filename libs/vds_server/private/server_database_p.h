#ifndef __VDS_PROTOCOLS_SERVER_DATABASE_P_H_
#define __VDS_PROTOCOLS_SERVER_DATABASE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_database.h"
#include "database.h"
#include "database_orm.h"

namespace vds {
  class istorage_log;
  
  class _server_database : public iserver_database
  {
  public:
    _server_database();
    ~_server_database();

    void start(const service_provider & sp);
    void stop(const service_provider & sp);


    void add_object(
      const service_provider & sp,
      database_transaction & tr,
      const principal_log_new_object & index);

    void get_endpoints(
      const service_provider & sp,
      database_transaction & tr,
      std::map<std::string, std::string> & addresses);
    
    void add_endpoint(
      const service_provider & sp,
      database_transaction & tr,
      const std::string & endpoint_id,
      const std::string & addresses);


    database * get_db() { return &this->db_; }
  private:
    database db_;
    
    class object_table : public database_table
    {
    public:
      object_table()
      : database_table("object"),
        object_id(this, "object_id"),
        length(this, "length"),
        meta_info(this, "meta_info")
      {
      }
      
      database_column<guid> object_id;
      database_column<size_t> length;
      database_column<const_data_buffer> meta_info;
    };
    
    class endpoint_table : public database_table
    {
    public:
      endpoint_table()
      : database_table("endpoint"),
        endpoint_id(this, "endpoint_id"),
        addresses(this, "addresses")
      {
      }
      
      database_column<std::string> endpoint_id;
      database_column<std::string> addresses;
    };
  };

  inline _server_database * vds::iserver_database::operator->()
  {
    return static_cast<_server_database *>(this);
  }
}

#endif // __VDS_PROTOCOLS_SERVER_DATABASE_P_H_
