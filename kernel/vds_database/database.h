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

  class database_table;
  class database_sql_builder;

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
    void execute(const std::string & sql);
    sql_statement parse(const std::string & sql);

    template<typename... column_types>
    database_sql_builder select(column_types &... columns);

  private:
    friend class _database;
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

  private:
    _database * const impl_;
  };

  
  class database_column_base
  {
  public:
    database_column_base(
      const database_table * owner,
      const std::string & name)
      : owner_(owner), name_(name), st_(nullptr)
    {
    }

    const database_table * owner() const { return this->owner_; }
    const std::string & name() const { return this->name_; }

  private:
    const database_table * const owner_;
    const std::string name_;

  protected:
    friend class database_sql_builder;
    int index_;
    sql_statement * st_;
  };


  class _database_expression
  {
  public:
  };

  template<typename value_type>
  class database_expression_equ : public _database_expression
  {
  public:
    database_expression_equ(
      database_column_base * column,
      const value_type & value)
      : column_(column), value_(value)
    {
    }

  private:
    database_column_base * column_;
    value_type value_;
  };

  class database_logical_and : public _database_expression
  {
  public:
    database_logical_and(
      std::unique_ptr<_database_expression> && left,
      std::unique_ptr<_database_expression> && right)
      : left_(std::move(left)), right_(std::move(right))
    {
    }

  private:
    std::unique_ptr<_database_expression> left_;
    std::unique_ptr<_database_expression> right_;
  };

  class database_expression
  {
  public:
    database_expression()
    {

    }
    database_expression(database_expression && origin)
      : impl_(std::move(origin.impl_))
    {
    }

    database_expression(std::unique_ptr<_database_expression> && impl)
      : impl_(std::move(impl))
    {
    }

    database_expression & operator = (database_expression && exp)
    {
      this->impl_ = std::move(exp.impl_);
      return *this;
    }

    database_expression operator && (database_expression && exp)
    {
      return database_expression(std::make_unique<database_logical_and>(std::move(this->impl_), std::move(exp.impl_)));
    }
  private:
    std::unique_ptr<_database_expression> impl_;
  };


  class database_reader
  {
  public:
    database_reader(sql_statement && st)
      : st_(std::move(st))
    {
    }

    bool read() { return this->st_.execute(); }


  private:
    sql_statement st_;
  };
  
  class database_sql_builder
  {
  public:
    database_sql_builder(database_transaction & t)
      : t_(t)
    {
    }

    database_sql_builder & where(database_expression && cond)
    {
      this->cond_ = std::move(cond);
      return *this;
    }

    database_sql_builder & select(database_column_base & column)
    {
      if (this->aliases_.end() == this->aliases_.find(column.owner())) {
        this->aliases_[column.owner()] = "t" + std::to_string(this->aliases_.size());
      }

      this->columns_.push_back(&column);
      return *this;
    }

    template<typename... column_types>
    database_sql_builder & select(database_column_base & column, column_types &... columns)
    {
      return this->select(column).select(columns);
    }

    database_reader get_reader() const
    {
      std::string sql = "SELECT ";

      int index = 0;
      for (auto column : this->columns_) {
        if (0 < index) {
          sql += ",";
        }

        sql += this->get_alias(column->owner());
        sql += ".";
        sql += column->name();

        column->index_ = index++;
      }

      sql += " FROM ";
      if (1 != this->aliases_.size()) {
        throw std::runtime_error("Not implemented");
      }

      auto st = this->t_.parse(sql);
      return database_reader(std::move(st));
    }

  private:
    database_transaction & t_;
    std::map<const database_table *, std::string> aliases_;
    std::list<database_column_base *> columns_;
    database_expression cond_;

    const std::string & get_alias(const database_table * t) const
    {
      return this->aliases_.find(t)->second;
    }
  };

  class database_table
  {
  public:
    database_table(const std::string & table_name)
    {
    }


  };
  
  template <typename value_type>
  class database_column : public database_column_base
  {
  public:
    database_column(
      const database_table * owner,
      const std::string & name)
      : database_column_base(owner, name)
    {
    }

    value_type get() const {
      value_type result;
      this->st_->get_value(this->index_, result);
      return result;
    }

    database_expression operator == (const value_type & value) const {
      return database_expression(std::make_unique<database_expression_equ<value_type>>(this, value));
    }
  };

  template<typename... column_types>
  inline database_sql_builder database_transaction::select(column_types &... columns)
  {
    return std::move(database_sql_builder(*this).select(columns...));
  }

}

#endif//__VDS_DATABASE_DATABASE_H_