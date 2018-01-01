#ifndef __VDS_DATABASE_DATABASE_H_
#define __VDS_DATABASE_DATABASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <list>
#include <map>
#include <string>
#include <mutex>

#include "guid.h"
#include "filename.h"
#include "async_task.h"

namespace vds {
  class _database;
  class _database_transaction;
  class _sql_statement;
  
  //orm
  template <typename command_type>
  class sql_command_builder;

  class sql_statement
  {
  public:
    sql_statement(_sql_statement * impl);
    sql_statement(sql_statement && original);
    ~sql_statement();

    void set_parameter(int index, int value);
    void set_parameter(int index, uint64_t value);
    void set_parameter(int index, const std::string & value);
    void set_parameter(int index, const guid & value);
    void set_parameter(int index, const const_data_buffer & value);

    bool execute();

    bool get_value(int index, int & value);
    bool get_value(int index, uint64_t & value);
    bool get_value(int index, std::string & value);
    bool get_value(int index, guid & value);
    bool get_value(int index, const_data_buffer & value);
    bool get_value(int index, double & value);

    sql_statement & operator = (sql_statement && original);

  private:
    _sql_statement * impl_;
  };

  class database_transaction
  {
  public:
    void execute(const char * sql);
    sql_statement parse(const char * sql);

    template <typename command_type>
    void execute(const command_type & command)
    {
      auto st = sql_command_builder<command_type>().build(*this, command);
      st.execute();      
    }
    
    template <typename command_type>
    sql_statement get_reader(const command_type & command)
    {
      return sql_command_builder<command_type>().build(*this, command);
    }

  private:
    friend class _database;
    friend class database;
   
    database_transaction()
      : impl_(nullptr)
    {
    }
    
    database_transaction(_database * impl)
      : impl_(impl)
    {
    }

    _database * const impl_;
  };

  class database
  {
  public:
    database();
    ~database();

    void open(const service_provider & sp, const filename & fn);
    void close();

    void async_transaction(
      const service_provider & sp,
      const std::function<bool(database_transaction & tr)> & callback);
    
    void sync_transaction(
      const service_provider & sp,
      const std::function<bool(database_transaction & tr)> & callback);

    async_task<> prepare_to_stop(const service_provider &sp);

  private:
    _database * const impl_;
  };
}

#endif//__VDS_DATABASE_DATABASE_H_