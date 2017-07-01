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
  class database_insert_builder;
  
  template<typename expression_type>
  class database_delete_builder;
    
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
    friend class database_reader_builder_base;
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
  
  class database_column_exp
  {
  public:
    database_column_exp(
      const database_column_base * column)
      : column_(column)
    {
    }

    std::string visit(database_sql_builder & builder) const
    {
      return builder.get_alias(this->column_->owner()) + "." + this->column_->name();
    }
    
  private:
    const database_column_base * column_;
  };

  class db_simple_column
  {
  public:
    db_simple_column(
      database_column_base & column)
      : column_(&column)
    {
    }

    std::string visit(database_sql_builder & builder) const
    {
      return builder.get_alias(this->column_->owner()) + "." + this->column_->name();
    }

  private:
    database_column_base * column_;
  };

  template <typename source_type>
  class _db_max
  {
  public:
    _db_max(
      source_type && column)
      : column_(std::move(column))
    {

    }

    std::string visit(database_sql_builder & builder) const
    {
      return "MAX(" + this->column_.visit(builder) + ")";
    }

  private:
    source_type column_;
  };

  template <typename source_type>
  class _db_length
  {
  public:
    _db_length(
      source_type && column)
      : column_(std::move(column))
    {

    }

    std::string visit(database_sql_builder & builder) const
    {
      return "LENGTH(" + this->column_.visit(builder) + ")";
    }

  private:
    source_type column_;
  };

  template <typename source_type>
  class _db_sum
  {
  public:
    _db_sum(
      source_type && column)
      : column_(std::move(column))
    {

    }

    std::string visit(database_sql_builder & builder) const
    {
      return "SUM(" + this->column_.visit(builder) + ")";
    }

  private:
    source_type column_;
  };

  inline _db_max<db_simple_column> db_max(database_column_base & column)
  {
    return _db_max<db_simple_column>(db_simple_column(column));
  }

  template <typename source_type>
  inline _db_max<source_type> db_max(source_type && column, typename std::enable_if<std::is_base_of<database_column_base, source_type>::value>::type)
  {
    return _db_max<source_type>(std::move(column));
  }

  inline _db_length<db_simple_column> db_length(database_column_base & column)
  {
    return _db_length<db_simple_column>(db_simple_column(column));
  }

  template <typename source_type>
  inline _db_length<source_type> db_length(source_type && column)
  {
    return _db_length<source_type>(std::move(column));
  }

  inline _db_sum<db_simple_column> db_sum(database_column_base & column)
  {
    return _db_sum<db_simple_column>(db_simple_column(column));
  }

  template <typename source_type>
  inline _db_sum<source_type> db_sum(source_type && column)
  {
    return _db_sum<source_type>(std::move(column));
  }

  template<typename value_type>
  class database_value_exp
  {
  public:
    database_value_exp(
      const value_type & value)
      : value_(value)
    {
    }

    std::string visit(database_sql_builder & builder) const
    {
      return builder.add_parameter(
        [value = this->value_](sql_statement & st, int index){
          st.set_parameter(index, value);
        });
    }
    
  private:
    value_type value_;
  };

  template<typename left_exp_type, typename right_exp_type>
  class database_logical_and;

  template<typename implementation_type, typename left_exp_type, typename right_exp_type>
  class database_binary_expression
  {
  public:
    database_binary_expression(
      left_exp_type && left,
      right_exp_type && right)
      : left_(std::move(left)), right_(std::move(right))
    {
    }

    template <typename other_exp>
    database_logical_and<implementation_type, other_exp> operator && (other_exp && exp);

  protected:
    left_exp_type left_;
    right_exp_type right_;
  };

  template<typename left_exp_type, typename right_exp_type>
  class database_expression_equ_exp : public database_binary_expression<database_expression_equ_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>
  {
    using base_class = database_binary_expression<database_expression_equ_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>;
  public:
    database_expression_equ_exp(
      left_exp_type && left,
      right_exp_type && right)
      : base_class(std::move(left), std::move(right))
    {
    }
    
    std::string visit(database_sql_builder & builder) const
    {
      return this->left_.visit(builder) + "=" + this->right_.visit(builder);
    }
  };

  template<typename left_exp_type, typename right_exp_type>
  class database_logical_and : public database_binary_expression<database_logical_and<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>
  {
    using base_class = database_binary_expression<database_logical_and<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>;
  public:
    database_logical_and(
      left_exp_type && left,
      right_exp_type && right)
      : base_class(std::move(left), std::move(right))
    {
    }
    
    std::string visit(database_sql_builder & builder) const
    {
      return "(" + this->left_.visit(builder) + ") AND (" + this->right_.visit(builder) + ")";
    }
  };

  template <typename condition_type>
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
      condition_type && cond)
    : type_(t), table_(table), cond_(std::move(cond))
    {
    }
    
    join_type type() const { return this->type_; }
    const database_table * table() const { return this->table_; }
    const condition_type & cond() const { return this->cond_; }
    
  public:
    join_type type_;
    const database_table * table_;
    condition_type cond_;
  };

  template <typename base_builder, typename condition_type>
  class database_reader_builder_with_join;

  template <typename base_builder, typename condition_type>
  class database_reader_builder_with_where;

  template <typename select_type>
  class database_reader_builder
  {
    using this_class = database_reader_builder;
  public:
    database_reader_builder(
      database_transaction & t,
      select_type && columns,
      database_table * first_table)
      : t_(t), columns_(columns), first_table_(first_table)
    {
    }

    database_reader_builder(
      database_reader_builder && origin)
      : t_(origin.t_), columns_(std::move(origin.columns_)), first_table_(origin.first_table_)
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      aliases[this->first_table_] = "t0";
    }

    std::string collect_sources(database_sql_builder & builder) const;

    std::string collect_condition(database_sql_builder & ) const
    {
      return std::string();
    }

    template <typename join_condition_type>
    database_reader_builder_with_join<this_class, join_condition_type> inner_join(const database_table & t, join_condition_type && cond)
    {
      return database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond));
    }

    template <typename where_condition_type>
    database_reader_builder_with_where<this_class, where_condition_type> where(where_condition_type && cond)
    {
      return database_reader_builder_with_where<this_class, where_condition_type>(std::move(*this), std::move(cond));
    }

    sql_statement get_reader() const {
      std::map<const database_table *, std::string> aliases;
      this->collect_aliases(aliases);

      database_sql_builder builder(aliases);
      std::string sql = "SELECT ";
      sql += this->columns_.generate_select(builder);
      sql += this->collect_sources(builder);
      sql += this->collect_condition(builder);

      return builder.set_parameters(this->t_.parse(sql));
    }

  private:
    database_transaction & t_;
    select_type columns_;
    database_table * first_table_;
  };

  template <typename base_builder, typename condition_type>
  class database_reader_builder_with_where : public base_builder
  {
    using this_class = database_reader_builder_with_where<base_builder, condition_type>;
  public:
    database_reader_builder_with_where(
      base_builder && b,
      condition_type && cond)
      : base_builder(std::move(b)),
      cond_(std::move(cond))
    {
    }

    std::string collect_condition(database_sql_builder & builder) const
    {
      return " WHERE " + this->cond_.visit(builder);
    }

    sql_statement get_reader() const {
      std::map<const database_table *, std::string> aliases;
      this->collect_aliases(aliases);

      database_sql_builder builder(aliases);
      std::string sql = "SELECT ";
      sql += this->columns_.generate_select(builder);
      sql += this->collect_sources(builder);
      sql += this->collect_condition(builder);

      return builder.set_parameters(this->t_.parse(sql));
    }

  private:
    condition_type cond_;
  };

  template <typename base_builder, typename condition_type>
  class database_reader_builder_with_join : public base_builder
  {
    using this_class = database_reader_builder_with_join;
  public:
    database_reader_builder_with_join(
      base_builder && b,
      const database_table * table,
      condition_type && cond)
      : base_builder(std::move(b)),
        table_(table),
        cond_(std::move(cond))
    {
    }

    template <typename join_condition_type>
    database_reader_builder_with_join<this_class, join_condition_type> inner_join(const database_table & t, join_condition_type && cond)
    {
      return database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond));
    }

    template <typename where_condition_type>
    database_reader_builder_with_where<this_class, where_condition_type> where(where_condition_type && cond)
    {
      return database_reader_builder_with_where<this_class, where_condition_type>(std::move(*this), std::move(cond));
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      base_builder::collect_aliases(aliases);

      aliases[this->table_] = "t" + std::to_string(aliases.size());
    }

    std::string collect_sources(database_sql_builder & builder) const
    {
      return base_builder::collect_sources(builder)
        + " INNER JOIN " + this->table_->name() + " " + builder.get_alias(this->table_)
        + " ON " + this->cond_.visit(builder);
    }

    sql_statement get_reader() const {
      std::map<const database_table *, std::string> aliases;
      this->collect_aliases(aliases);

      database_sql_builder builder(aliases);
      std::string sql = this->prefix_;
      sql += "SELECT ";
      sql += this->columns_.generate_select(builder);
      sql += this->collect_sources(builder);
      sql += this->collect_condition(builder);

      return builder.set_parameters(this->t_.parse(sql));
    }

  private:
    const database_table * table_;
    condition_type cond_;
  };


  template<typename column_type>
  class database_select_builder_base
  {
  public:
    database_select_builder_base(
      column_type && column)
      : column_(column)
    {
    }


  private:
    column_type column_;
  };

  template<typename column_type, typename dummy = void>
  class database_column_holder;

  template<typename column_type>
  class database_column_holder<column_type, typename std::enable_if<std::is_base_of<database_column_base, typename std::remove_reference<column_type>::type>::value>::type>
  {
  public:
    using holder = db_simple_column;
  };

  template<typename column_type>
  class database_column_holder<column_type, typename std::enable_if<!std::is_base_of<database_column_base, typename std::remove_reference<column_type>::type>::value>::type>
  {
  public:
    using holder = column_type;
  };

  template<typename... column_types>
  class database_select_builder;
  
  template<typename column_type>
  class database_select_builder<column_type>
  {
  public:
    database_select_builder(database_table * t, column_type && column)
      : t_(t), column_(std::forward<column_type>(column))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      aliases[this->t_] = "t0";
    }
    
    std::string collect_sources(database_sql_builder & builder) const
    {
      return " FROM " + this->t_->name() + " " + builder.get_alias(this->t_);
    }
    
    std::string generate_select(database_sql_builder & builder) const
    {
      return this->column_.visit(builder);
    }

  protected:
    database_table * t_;
    typename database_column_holder<column_type>::holder column_;
  };

  template<typename column_type, typename... column_types>
  class database_select_builder<column_type, column_types...> : public database_select_builder<column_types...>
  {
    using base_class = database_select_builder<column_types...>;
    using this_class = database_select_builder<column_type, column_types...>;
  public:
    database_select_builder(database_table * t, column_type && column, column_types &&... columns)
      : base_class(t, std::forward<column_types>(columns)...),
        column_(std::forward<column_type>(column))
    {
    }

    template <typename join_condition_type>
    database_reader_builder_with_join<this_class, join_condition_type> inner_join(const database_table & t, join_condition_type && cond)
    {
      return database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond));
    }

    std::string generate_select(database_sql_builder & builder) const
    {
      return this->column_.visit(builder) + "," + base_class::generate_select(builder);
    }

  private:
    typename database_column_holder<column_type>::holder column_;
  };
  
  class database_table
  {
  public:
    database_table(const std::string & table_name)
    : name_(table_name)
    {
    }
    
    const std::string & name() const { return this->name_; }
    
    template<typename... column_types>
    database_select_builder<column_types...> select(column_types &&... columns)
    {
      return database_select_builder<column_types...>(this, std::forward<column_types>(columns)...);
    }
    
    template<typename... setter_types>
    database_insert_builder insert(setter_types &&... setters);
    
    template<typename expression_type>
    database_delete_builder<expression_type> delete_if(expression_type && cond);

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

    database_expression_equ_exp<database_column_exp, database_value_exp<value_type>> operator == (value_type value) const {
      return database_expression_equ_exp<database_column_exp, database_value_exp<value_type>>(
          database_column_exp(this),
          database_value_exp<value_type>(value));
    }
    
    database_expression_equ_exp<database_column_exp, database_column_exp> operator == (const database_column<value_type> & right) const {
      return database_expression_equ_exp<database_column_exp, database_column_exp>(
          database_column_exp(this),
          database_column_exp(&right));
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
  
  /////////////////////////////// Insert ////////////////////////
  class database_insert_builder
  {
  public:
    
    template<typename... setter_types>
    database_insert_builder(const database_table & table, setter_types &&... setters)
    : table_(table)
    {
      this->set(std::forward<setter_types>(setters)...);
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
    
  private:
    friend class sql_command_builder<database_insert_builder>;
    
    const database_table & table_;
    std::list<const database_column_base *> columns_;
    std::list<std::function<void(sql_statement & st, int index)>> values_;
  };
  
  /////////////////////////////// Insert from ////////////////////////
  class database_insert_from_builder
  {
  public:
    database_insert_from_builder(database_transaction & t, const database_table & table)
    : t_(t), table_(table)
    {
    }
    
    template<typename... column_types>
    database_insert_from_builder & set(const database_column_base * column, column_types... columns)
    {
      return this->set(column).set(columns...);
    }
   
    database_insert_from_builder & set(const database_column_base * column)
    {
      this->columns_.push_back(column);
      return *this;
    }
    
    template<typename... column_types>
    database_select_builder<column_types...> select(column_types &&... columns)
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
      sql += ") ";
      
      return database_select_builder<column_types...>(this->t_, sql, std::forward<column_types>(columns)...);
    }
    
  private:
    database_transaction & t_;
    const database_table & table_;
    std::list<const database_column_base *> columns_;
  };
 

  /////////////////////////////// Delete ////////////////////////
  template <typename condition_type>
  class database_delete_builder
  {
  public:
    database_delete_builder(
      const database_table & table,
      condition_type && cond)
      : table_(table),
        cond_(std::move(cond))
    {
    }

  private:
    friend class sql_command_builder<database_delete_builder>;
    const database_table & table_;
    condition_type cond_;
  };
  
  template<typename implementation_type, typename left_exp_type, typename right_exp_type>
  template<typename other_exp>
  inline database_logical_and<implementation_type, other_exp> database_binary_expression<implementation_type, left_exp_type, right_exp_type>::operator&&(other_exp && exp)
  {
    return database_logical_and<implementation_type, other_exp>(std::move(*static_cast<implementation_type *>(this)), std::move(exp));
  }

  template<typename select_type>
  inline std::string database_reader_builder<select_type>::collect_sources(database_sql_builder & builder) const
  {
    return " FROM " + this->first_table_->name() + " " + builder.get_alias(this->first_table_);
  }

  
  template <>
  class sql_command_builder<database_insert_builder>
  {
  public:
    void execute(database_transaction & t, const database_insert_builder & builder)
    {
      std::string sql = "INSERT INTO " + builder.table_.name() + "(";
      bool is_first = true;
      for(auto column : builder.columns_){
        if(is_first){
          is_first = false;
        }
        else {
          sql += ",";
        }
        sql += column->name();
      }
      sql += ") VALUES (";
      
      for(size_t index = 0; index < builder.columns_.size(); ++index){
        if(0 < index){
          sql += ",";
        }
        sql += "@p" + std::to_string(index);
      }
      sql += ")";
      
      auto st = t.parse(sql.c_str());
      int index = 0;
      for(auto & set_value : builder.values_){
        set_value(st, index++);
      }
      
      st.execute();
    }
  };
  
  template<typename expression_type>
  class sql_command_builder<database_delete_builder<expression_type>>
  {
  public:
    
    void execute(database_transaction & t, const database_delete_builder<expression_type> & delete_builder)
    {
      std::map<const database_table *, std::string> aliases;
      aliases[&delete_builder.table_] = delete_builder.table_.name();

      database_sql_builder builder(aliases);
      std::string sql = "DELETE FROM " + delete_builder.table_.name() + " WHERE " + delete_builder.cond_.visit(builder);
      builder.set_parameters(t.parse(sql.c_str())).execute();
    }
  };
  
  
  template<typename... setter_types>
  inline database_insert_builder database_table::insert(setter_types &&... setters)
  {
    return database_insert_builder(*this, std::forward<setter_types>(setters)...);
  }
  
  template <typename source_type>
  class sql_reader_builder
  {
  public:
    
    sql_statement execute(database_transaction & t, const source_type & source)
    {
      std::map<const database_table *, std::string> aliases;
      source.collect_aliases(aliases);

      database_sql_builder builder(aliases);
      std::string sql = "SELECT "
        + source.generate_select(builder)
        + source.collect_sources(builder)
        + source.collect_condition(builder);

      return builder.set_parameters(t.parse(sql.c_str()));
    }
  };
}

#endif // __VDS_DATABASE_DATABASE_ORM_H_
