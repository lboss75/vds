#ifndef __VDS_DATABASE_DATABASE_ORM_H_
#define __VDS_DATABASE_DATABASE_ORM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database.h"

namespace vds {
  class database_table;
  class database_sql_builder;

    
  class database_column_base
  {
  public:
    database_column_base(
      const database_table * owner,
      const std::string & name)
      : owner_(owner), name_(name)
    {
    }

    const database_table * owner() const { return this->owner_; }
    const std::string & name() const { return this->name_; }

  private:
    const database_table * const owner_;
    const std::string name_;

  protected:
    friend class database_reader_builder;
    int index_;
  };
  
  class database_column_setter
  {
  public:
    database_column_setter(
      const database_column_base * column,
      const std::function<void(sql_statement & st, int index)> & set_parameter)
    : column_(column),
      set_parameter_(set_parameter)
    {
    }
    
    const database_column_base * column() const { return this->column_; }
    const std::function<void(sql_statement & st, int index)> & set_parameter() const { return this->set_parameter_; }
    
  private:
    const database_column_base * column_;
    std::function<void(sql_statement & st, int index)> set_parameter_;
  };


  class database_sql_builder
  {
  public:
    database_sql_builder(
      const std::map<const database_table *, std::string> & aliases)
    : aliases_(aliases)
    {
    }
   
    const std::string & get_alias(const database_table * t) const;
    
    std::string add_parameter(const std::function<void(sql_statement & st, int index)> & set_parameter)
    {
      this->set_parameters_.push_back(set_parameter);
      return "@p" + std::to_string(this->set_parameters_.size());
    }
    
    sql_statement set_parameters(sql_statement && st)
    {
      int index = 0;
      for(auto & p : this->set_parameters_){
        p(st, index++);
      }
      
      return std::move(st);
    }
    
  private:
    const std::map<const database_table *, std::string> & aliases_;
    std::list<std::function<void(sql_statement & st, int index)>> set_parameters_;
  };
  
  class _database_expression
  {
  public:
    virtual ~_database_expression() {}
    
    virtual std::string visit(database_sql_builder & builder) = 0;
  };

  class database_column_exp : public _database_expression
  {
  public:
    database_column_exp(
      const database_column_base * column)
      : column_(column)
    {
    }

    std::string visit(database_sql_builder & builder) override
    {
      return builder.get_alias(this->column_->owner()) + "." + this->column_->name();
    }
    
  private:
    const database_column_base * column_;
  };
  
  template<typename value_type>
  class database_value_exp : public _database_expression
  {
  public:
    database_value_exp(
      const value_type & value)
      : value_(value)
    {
    }

    std::string visit(database_sql_builder & builder) override
    {
      return builder.add_parameter(
        [value = this->value_](sql_statement & st, int index){
          st.set_parameter(index, value);
        });
    }
    
  private:
    value_type value_;
  };

  class database_expression_equ_exp : public _database_expression
  {
  public:
    database_expression_equ_exp(
      std::unique_ptr<_database_expression> && left,
      std::unique_ptr<_database_expression> && right)
      : left_(std::move(left)), right_(std::move(right))
    {
    }
    
    std::string visit(database_sql_builder & builder) override
    {
      return this->left_->visit(builder) + "=" + this->right_->visit(builder);
    }

  private:
    std::unique_ptr<_database_expression> left_;
    std::unique_ptr<_database_expression> right_;
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
    
    std::string visit(database_sql_builder & builder) override
    {
      return "(" + this->left_->visit(builder) + ") AND (" + this->right_->visit(builder) + ")";
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
    
    operator bool () const { return !!this->impl_; }
    
    _database_expression * operator ->() const { return this->impl_.get(); }
    
  private:
    std::unique_ptr<_database_expression> impl_;
  };


  class database_join_expression
  {
  public:
    enum class join_type
    {
      inner_join
    };
    
    database_join_expression(
      join_type t,
      const database_table * table,
      database_expression && cond)
    : type_(t), table_(table), cond_(std::move(cond))
    {
    }
    
    join_type type() const { return this->type_; }
    const database_table * table() const { return this->table_; }
    const database_expression & cond() const { return this->cond_; }
    
  public:
    join_type type_;
    const database_table * table_;
    database_expression cond_;    
  };
  
  class database_reader_builder
  {
  public:
    database_reader_builder(
      database_transaction & t,
      const std::list<database_column_base *> & columns,
      database_table * first_table)
      : t_(t), columns_(columns), first_table_(first_table)
    {
    }

    database_reader_builder & inner_join(const database_table & t, database_expression && cond)
    {
      this->joins_.push_back(database_join_expression(database_join_expression::join_type::inner_join, &t, std::move(cond)));
      return *this;
    }

    database_reader_builder & where(database_expression && cond)
    {
      this->cond_ = std::move(cond);
      return *this;
    }
    
    sql_statement get_reader() const;

  private:
    database_transaction & t_;
    std::list<database_column_base *> columns_;
    database_table * first_table_;
    std::list<database_join_expression> joins_;
    database_expression cond_;
  };

  class database_select_builder
  {
  public:
    database_select_builder(database_transaction & t)
      : t_(t)
    {
    }


    database_select_builder & select(database_column_base & column)
    {
      this->columns_.push_back(&column);
      return *this;
    }

    template<typename... column_types>
    database_select_builder & select(database_column_base & column, column_types &... columns)
    {
      return this->select(column).select(columns...);
    }

    database_reader_builder from(database_table & t) const
    {
      return database_reader_builder(this->t_, this->columns_, &t);
    }

  private:
    database_transaction & t_;
    std::list<database_column_base *> columns_;
  };
  
  class database_table
  {
  public:
    database_table(const std::string & table_name)
    : name_(table_name)
    {
    }
    
    const std::string & name() const { return this->name_; }

  private:
    std::string name_;
  };
  
  template <typename value_type, typename db_value_type = value_type>
  class database_column : public database_column_base
  {
  public:
    database_column(
      const database_table * owner,
      const std::string & name)
      : database_column_base(owner, name)
    {
    }

    value_type get(sql_statement & st) const {
      db_value_type result;
      st.get_value(this->index_, result);
      return (value_type)result;
    }

    database_expression operator == (const value_type & value) const {
      return database_expression(
        std::make_unique<database_expression_equ_exp>(
          std::make_unique<database_column_exp>(this),
          std::make_unique<database_value_exp<value_type>>(value)));
    }
    
    database_expression operator == (const database_column<value_type> & right) const {
      return database_expression(
        std::make_unique<database_expression_equ_exp>(
          std::make_unique<database_column_exp>(this),
          std::make_unique<database_column_exp>(&right)));
    }
    
    database_expression operator == (database_expression && right) const {
      return database_expression(std::make_unique<database_expression_equ_exp>(this, right));
    }
    
    database_column_setter operator = (const value_type & value) const {
      return database_column_setter(this, [val = (db_value_type)value](sql_statement & st, int index) {
        st.set_parameter(index, val);
      });
    }
  };

  inline const std::string & database_sql_builder::get_alias(const database_table * t) const
  {
    auto p = this->aliases_.find(t);
    if(this->aliases_.end() == p){
      throw std::runtime_error("Table " + t->name() + " not found");
    }
    
    return p->second;
  }
  
  template<typename... column_types>
  inline database_select_builder database_transaction::select(column_types &... columns)
  {
    return std::move(database_select_builder(*this).select(columns...));
  }
    
  inline sql_statement database_reader_builder::get_reader() const
  {
    std::map<const database_table *, std::string> aliases;
    aliases[this->first_table_] = "t0";
    for(auto & j : this->joins_){
      if(aliases.end() == aliases.find(j.table())){
        aliases[j.table()] = "t" + std::to_string(aliases.size());
      }
    }
    
    database_sql_builder builder(aliases);
    std::string sql = "SELECT ";

    int index = 0;
    for (auto column : this->columns_) {
      if (0 < index) {
        sql += ",";
      }

      sql += builder.get_alias(column->owner());
      sql += ".";
      sql += column->name();

      column->index_ = index++;
    }

    sql += " FROM " + this->first_table_->name() + " " + builder.get_alias(this->first_table_);
    
    for(auto & j : this->joins_){
      switch(j.type()){
        case database_join_expression::join_type::inner_join:
          sql += " INNER JOIN " + j.table()->name() + " " + builder.get_alias(j.table());
          sql += " ON " + j.cond()->visit(builder);
          break;
      }
    }
    
    if(this->cond_){
      sql += " WHEN " + this->cond_->visit(builder);
    }
      

    return builder.set_parameters(this->t_.parse(sql));
  }
  
  /////////////////////////////// Insert ////////////////////////
  class database_insert_builder
  {
  public:
    database_insert_builder(database_transaction & t, const database_table & table)
    : t_(t), table_(table)
    {
    }
    
    template<typename... setter_types>
    database_insert_builder & set(database_column_setter && setter, setter_types &&... setters)
    {
      return this->set(std::move(setter)).set(std::move(setters)...);
    }
   
    database_insert_builder & set(database_column_setter && setter)
    {
      this->columns_.push_back(setter.column());
      this->values_.push_back(setter.set_parameter());
      return *this;
    }
    
    void execute()
    {
      std::string sql = "INSERT INTO " + this->table_.name() + "(";
      bool is_first = true;
      for(auto column : this->columns_){
        if(is_first){
          is_first = false;
        }
        else {
          sql += ",";
        }
        sql += column->name();
      }
      sql += ") VALUES (";
      
      for(size_t index = 0; index < this->columns_.size(); ++index){
        if(0 < index){
          sql += ",";
        }
        sql += "@p" + std::to_string(index);
      }
      sql += ")";
      
      auto st = this->t_.parse(sql);
      int index = 0;
      for(auto & set_value : this->values_){
        set_value(st, index++);
      }
      
      st.execute();
    }
    
  private:
    database_transaction & t_;
    const database_table & table_;
    std::list<const database_column_base *> columns_;
    std::list<std::function<void(sql_statement & st, int index)>> values_;
  };
  
  inline database_insert_builder database_transaction::insert_into(const database_table & table)
  {
    return database_insert_builder(*this, table);
  }
  /////////////////////////////// Delete ////////////////////////
  class database_delete_builder
  {
  public:
    database_delete_builder(database_transaction & t, const database_table & table)
    : t_(t), table_(table)
    {
    }
    
    database_delete_builder & where(database_expression && cond)
    {
      this->cond_ = std::move(cond);
      return *this;
    }
    
    void execute()
    {
      std::map<const database_table *, std::string> aliases;
      aliases[&this->table_] = this->table_.name();
      
      database_sql_builder builder(aliases);
      std::string sql = "DELETE FROM " + this->table_.name() + " WHERE " + this->cond_->visit(builder);
      builder.set_parameters(this->t_.parse(sql)).execute();
    }
    
  private:
    database_transaction & t_;
    const database_table & table_;
    database_expression cond_;
  };
  
  inline database_delete_builder database_transaction::delete_from(const database_table & table)
  {
    return database_delete_builder(*this, table);
  }
}

#endif // __VDS_DATABASE_DATABASE_ORM_H_
