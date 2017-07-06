#ifndef __VDS_PROTOCOLS_PRINCIPAL_MANAGER_P_H_
#define __VDS_PROTOCOLS_PRINCIPAL_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "principal_manager.h"
#include "database_orm.h"
#include "log_records.h"
#include "not_mutex.h"

namespace vds {
  class asymmetric_private_key;

  class _principal_manager
  {
  public:

    enum class principal_log_state
    {
      not_found = 0,
      stored = 1, //Just stored
      front = 2, //Stored + all parents processed
      processed = 3, //+ Processed
      tail = 4 //
    };

    
    static void create_database_objects(
      const service_provider & sp,
      uint64_t db_version,
      database_transaction & t);

    bool save_record(
      const service_provider & sp,
      const principal_log_record & record,
      const const_data_buffer & signature);
    
    bool get_record(
      const service_provider & sp,
      const principal_log_record::record_id & id,
      principal_log_record & result_record,
      const_data_buffer & result_signature);
    
    void get_unknown_records(
      const service_provider & sp,
      std::list<principal_log_record::record_id>& result);

  private:
    not_mutex principal_log_mutex_;

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

    class user_principal_table : public database_table
    {
    public:
      user_principal_table()
        : database_table("user_principal"),
        id(this, "id"),
        login(this, "login")
      {
      }

      database_column<guid> id;
      database_column<std::string> login;
    };
    
    class principal_log_table : public database_table
    {
    public:
      principal_log_table()
        : database_table("principal_log"),
        id(this, "id"),
        principal_id(this, "principal_id"),
        message(this, "message"),
        signature(this, "signature"),
        order_num(this, "order_num"),
        state(this, "state")
      {
      }

      database_column<guid> id;
      database_column<guid> principal_id;
      database_column<std::string> message;
      database_column<const_data_buffer> signature;
      database_column<int> order_num;
      database_column<int> state;
    };
    
    class principal_log_link_table : public database_table
    {
    public:
      principal_log_link_table()
        : database_table("principal_log_link"),
        parent_id(this, "parent_id"),
        follower_id(this, "follower_id")
      {
      }

      database_column<guid> parent_id;
      database_column<guid> follower_id;
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
    
    void principal_log_add_link(
      const service_provider & sp,
      const guid & source_id,
      const guid & target_id);
    
    void add_principal_log(
      const service_provider & sp,
      const guid & record_id,
      const guid & principal_id,
      const std::string & body,
      const const_data_buffer & signature,
      int order_num,
      principal_log_state state);
    
    void principal_log_update_state(
      const service_provider & sp,
      const principal_log_record::record_id & record_id,
      principal_log_state state);
    
    principal_log_state principal_log_get_state(
      const service_provider & sp,
      const principal_log_record::record_id & record_id);

    size_t get_current_state(
      const service_provider & sp,
      std::list<guid> & active_records);

    void principal_log_get_parents(
      const service_provider & sp,
      const principal_log_record::record_id & record_id,
      std::list<principal_log_record::record_id>& parents);

    void principal_log_get_followers(
      const service_provider & sp,
      const principal_log_record::record_id & record_id,
      std::list<principal_log_record::record_id>& followers);

    void processed_record(
      const service_provider & sp,
      const principal_log_record::record_id & id);

    bool get_front_record(
      const service_provider & sp,
      principal_log_record & result_record,
      const_data_buffer & result_signature);

    void delete_record(
      const service_provider & sp,
      const principal_log_record::record_id & id);

    bool get_record_by_state(
      const service_provider & sp,
      principal_log_state state,
      principal_log_record & result_record,
      const_data_buffer & result_signature);

    void get_principal_log(
      const service_provider & sp,
      const guid & principal_id,
      size_t last_order_num,
      size_t & result_last_order_num,
      std::list<principal_log_record> & records);

    principal_log_record add_local_record(
        const service_provider & sp,
        const principal_log_record::record_id & record_id,
        const guid & principal_id,
        const std::shared_ptr<json_value> & message,
        const asymmetric_private_key & principal_private_key,
        const_data_buffer & signature);
  };
}

#endif // __VDS_PROTOCOLS_PRINCIPAL_MANAGER_P_H_
