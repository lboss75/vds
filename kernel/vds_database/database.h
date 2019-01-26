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

#include "filename.h"
#include "const_data_buffer.h"
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
    sql_statement();
    sql_statement(_sql_statement * impl);
    sql_statement(sql_statement && original);
    ~sql_statement();

    void set_parameter(int index, int value);
    void set_parameter(int index, int64_t value);
    void set_parameter(int index, const std::string & value);
    void set_parameter(int index, const const_data_buffer & value);
    void set_parameter(int index, const std::chrono::system_clock::time_point & value);

    expected<bool> execute();

    bool get_value(int index, int & value);
    bool get_value(int index, int64_t & value);
    bool get_value(int index, std::string & value);
    bool get_value(int index, const_data_buffer & value);
    bool get_value(int index, double & value);
    bool get_value(int index, std::chrono::system_clock::time_point & value);

    bool is_null(int index) const;

    sql_statement & operator = (sql_statement && original);

  private:
    _sql_statement * impl_;
  };

  class database_read_transaction {
  public:
    expected<sql_statement> parse(const char * sql) const;

    template <typename command_type>
    expected<sql_statement> get_reader(const command_type & command) const
    {
      return sql_command_builder<command_type>().build(*this, command);
    }

  protected:
    friend class _database;
    friend class database;

    database_read_transaction(const std::shared_ptr<_database> & impl)
      : impl_(impl) {
    }

    std::shared_ptr<_database> impl_;
  };

  class database_transaction : public database_read_transaction
  {
  public:
    expected<void> execute(const char * sql);

    template <typename command_type>
    expected<void> execute(const command_type & command)
    {
      GET_EXPECTED(st, sql_command_builder<command_type>().build(*this, command));
      CHECK_EXPECTED(st.execute());
      return expected<void>();
    }

    expected<int> rows_modified() const;
    expected<int> last_insert_rowid() const;

  private:
    friend class _database;
    friend class database;

    database_transaction(const std::shared_ptr<_database> & impl)
      : database_read_transaction(impl) {
    }
  };

  class database
  {
  public:
    database();
    ~database();

    expected<void> open(const service_provider * sp, const filename & fn);
    expected<void> close();

    vds::async_task<vds::expected<void>> async_transaction(      
      const std::function<expected<bool>(database_transaction & tr)> & callback);

    vds::async_task<vds::expected<void>> async_read_transaction(      
      const std::function<expected<void>(database_read_transaction & tr)> & callback);

    vds::async_task<vds::expected<void>> prepare_to_stop();

    size_t queue_length() const;

  private:
    std::shared_ptr<_database> impl_;
  };
}

#endif//__VDS_DATABASE_DATABASE_H_