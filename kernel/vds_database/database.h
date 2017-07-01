#ifndef __VDS_DATABASE_DATABASE_H_
#define __VDS_DATABASE_DATABASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <list>
#include <map>
#include <string>

#include "guid.h"
#include "filename.h"

namespace vds {
  class _database;
  class _database_transaction;
  class _sql_statement;
  
  //orm
  template <typename command_type>
  class sql_command_builder;
  
  template <typename command_type>
  class sql_reader_builder;

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
      sql_command_builder<command_type>().execute(*this, command);
    }
    
    template <typename command_type>
    sql_statement get_reader(const command_type & command)
    {
      return sql_reader_builder<command_type>().execute(*this, command);
    }

    static database_transaction & current(const service_provider & sp);

  private:
    friend class _database;
    friend class database;
    
    database_transaction()
    {
    }
    
    database_transaction(const std::shared_ptr<_database_transaction> & impl)
      : impl_(impl)
    {
    }

    std::shared_ptr<_database_transaction> impl_;
  };

  class database
  {
  public:
    database();
    ~database();

    void open(const filename & fn);
    void close();

    database_transaction begin_transaction();
    void commit(database_transaction & db);
    void rollback(database_transaction & db);

  private:
    _database * const impl_;
  };

  class database_transaction_scope
  {
  public:
    database_transaction_scope(const service_provider & sp, database & db);
    ~database_transaction_scope();

    void commit();

  private:
    database & db_;
    database_transaction transaction_;
    bool successful_;
  };
}

#endif//__VDS_DATABASE_DATABASE_H_