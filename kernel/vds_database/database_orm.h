#ifndef __VDS_DATABASE_DATABASE_ORM_H_
#define __VDS_DATABASE_DATABASE_ORM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database.h"

namespace vds {
  //forwards
  class database_table;
  class _database_sql_builder;
  
  template<typename base_builder, typename... column_types>
  class _database_select_builder;
  
  class _database_insert_builder;
  class _database_insert_from_builder;
  
  template<typename expression_type>
  class _database_delete_builder;

  class _database_column_setter;  
  class _database_column_exp;
  
  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_equ_exp;
  
  template<typename value_type>
  class _database_value_exp;
  
  class _database_source_base;
  
  //database table
  class database_table
  {
  public:
    database_table(const std::string & table_name)
    : name_(table_name)
    {
    }
    
    const std::string & name() const { return this->name_; }
    
    template<typename... column_types>
    _database_select_builder<_database_source_base, column_types...> select(column_types &&... columns);
    
    template<typename... setter_types>
    _database_insert_builder insert(setter_types &&... setters);
    
    template<typename... column_types>
    _database_insert_from_builder insert_into(column_types &&... columns);
    
    template<typename expression_type>
    _database_delete_builder<expression_type> delete_if(expression_type && cond);

  private:
    std::string name_;
  };  
    
  class _database_column_base
  {
  public:
    _database_column_base(
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
    int index_;
  };
  
  template <typename value_type, typename db_value_type = value_type>
  class database_column : public _database_column_base
  {
  public:
    database_column(
      const database_table * owner,
      const std::string & name)
      : _database_column_base(owner, name)
    {
    }

    value_type get(sql_statement & st) const;

    _database_expression_equ_exp<_database_column_exp, _database_value_exp<value_type>> operator == (value_type value) const;
    _database_expression_equ_exp<_database_column_exp, _database_column_exp> operator == (const database_column<value_type> & right) const;
    _database_column_setter operator = (const value_type & value) const;
  };
  
  ////////////////////////////////////////////
  class _database_column_setter
  {
  public:
    _database_column_setter(
      const _database_column_base * column,
      const std::function<void(sql_statement & st, int index)> & set_parameter)
    : column_(column),
      set_parameter_(set_parameter)
    {
    }
    
    const _database_column_base * column() const { return this->column_; }
    const std::function<void(sql_statement & st, int index)> & set_parameter() const { return this->set_parameter_; }
    
  private:
    const _database_column_base * column_;
    std::function<void(sql_statement & st, int index)> set_parameter_;
  };
  
  //////////////////////////////////////////////
  class _database_sql_builder
  {
  public:
    _database_sql_builder(
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
  
  class _database_column_exp
  {
  public:
    _database_column_exp(
      const _database_column_base * column)
      : column_(column)
    {
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return builder.get_alias(this->column_->owner()) + "." + this->column_->name();
    }
    
  private:
    const _database_column_base * column_;
  };

  class _db_simple_column
  {
  public:
    _db_simple_column(
      const _database_column_base & column)
      : column_(&column)
    {
    }
    
    _db_simple_column(
      _db_simple_column && column)
      : column_(column.column_)
    {
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return builder.get_alias(this->column_->owner()) + "." + this->column_->name();
    }

  private:
    const _database_column_base * column_;
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

    std::string visit(_database_sql_builder & builder) const
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

    std::string visit(_database_sql_builder & builder) const
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

    std::string visit(_database_sql_builder & builder) const
    {
      return "SUM(" + this->column_.visit(builder) + ")";
    }

  private:
    source_type column_;
  };

  template <typename source_type, typename dummy = typename std::enable_if<std::is_base_of<_database_column_base, source_type>::value>::type>
  inline _db_max<_db_simple_column> db_max(source_type & column)
  {
    return _db_max<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<!std::is_base_of<_database_column_base, source_type>::value>::type>
  inline _db_max<source_type> db_max(source_type && column)
  {
    return _db_max<source_type>(std::move(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<std::is_base_of<_database_column_base, source_type>::value>::type>
  inline _db_length<_db_simple_column> db_length(source_type & column)
  {
    return _db_length<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<!std::is_base_of<_database_column_base, source_type>::value>::type>
  inline _db_length<source_type> db_length(source_type && column)
  {
    return _db_length<source_type>(std::move(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<std::is_base_of<_database_column_base, source_type>::value>::type>
  inline _db_sum<_db_simple_column> db_sum(source_type & column)
  {
    return _db_sum<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<!std::is_base_of<_database_column_base, source_type>::value>::type>
  inline _db_sum<source_type> db_sum(source_type && column)
  {
    return _db_sum<source_type>(std::move(column));
  }

  template<typename value_type>
  class _database_value_exp
  {
  public:
    _database_value_exp(
      const value_type & value)
      : value_(value)
    {
    }

    std::string visit(_database_sql_builder & builder) const
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
  class _database_logical_and;

  template<typename implementation_type, typename left_exp_type, typename right_exp_type>
  class _database_binary_expression
  {
  public:
    _database_binary_expression(
      left_exp_type && left,
      right_exp_type && right)
      : left_(std::move(left)), right_(std::move(right))
    {
    }

    template <typename other_exp>
    _database_logical_and<implementation_type, other_exp> operator && (other_exp && exp);

  protected:
    left_exp_type left_;
    right_exp_type right_;
  };

  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_equ_exp : public _database_binary_expression<_database_expression_equ_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>
  {
    using base_class = _database_binary_expression<_database_expression_equ_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>;
  public:
    _database_expression_equ_exp(
      left_exp_type && left,
      right_exp_type && right)
      : base_class(std::move(left), std::move(right))
    {
    }
    
    std::string visit(_database_sql_builder & builder) const
    {
      return this->left_.visit(builder) + "=" + this->right_.visit(builder);
    }
  };

  template<typename left_exp_type, typename right_exp_type>
  class _database_logical_and : public _database_binary_expression<_database_logical_and<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>
  {
    using base_class = _database_binary_expression<_database_logical_and<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>;
  public:
    _database_logical_and(
      left_exp_type && left,
      right_exp_type && right)
      : base_class(std::move(left), std::move(right))
    {
    }
    
    std::string visit(_database_sql_builder & builder) const
    {
      return "(" + this->left_.visit(builder) + ") AND (" + this->right_.visit(builder) + ")";
    }
  };

  template <typename base_builder, typename condition_type>
  class _database_reader_builder_with_join;

  template <typename base_builder, typename condition_type>
  class _database_reader_builder_with_where;
  
  class _database_source_base
  {
  public:
    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
    }
    
    std::string start_sql(_database_sql_builder & builder) const
    {
      return std::string();
    }
    
    std::string generate_select(_database_sql_builder & builder) const
    {
      return std::string();
    }
    
    std::string collect_sources(_database_sql_builder & builder) const
    {
      return std::string();
    }
    
    std::string collect_condition(_database_sql_builder & builder) const
    {
      return std::string();
    }
    
    std::string final_sql(_database_sql_builder & builder) const
    {
      return std::string();
    }
  };
  /////////////////////////
  class _db_oder_column
  {
  public:
    _db_oder_column(const _database_column_base & column)
    : column_(&column)
    {
    }
    
    std::string visit(_database_sql_builder & builder) const
    {
      return builder.get_alias(this->column_->owner()) + "." + this->column_->name();
    }
    
  private:
    const _database_column_base * column_;
  };
  
  template<typename base_builder, typename... column_types>
  class _database_order_builder;
  
  template<typename column_type, typename dummy = void>
  class _database_order_holder;

  template<typename column_type>
  class _database_order_holder<column_type, typename std::enable_if<std::is_base_of<_database_column_base, typename std::remove_reference<column_type>::type>::value>::type>
  {
  public:
    using holder = _db_oder_column;
  };

  template<typename column_type>
  class _database_order_holder<column_type, typename std::enable_if<!std::is_base_of<_database_column_base, typename std::remove_reference<column_type>::type>::value>::type>
  {
  public:
    using holder = column_type;
  };

  template<typename base_builder, typename column_type>
  class _database_order_builder<base_builder, column_type> : public base_builder
  {
    using this_class = _database_order_builder<base_builder, column_type>;
  public:
    _database_order_builder(
      base_builder && b,
      column_type && column)
      : base_builder(std::move(b)),
        column_(std::forward<column_type>(column))
    {
    }

    std::string final_sql(_database_sql_builder & builder) const
    {
      return " ORDER BY " + this->generate_columns(builder);
    }

    std::string generate_columns(_database_sql_builder & builder) const
    {
      return this->column_.visit(builder);
    }

  protected:
    typename _database_order_holder<column_type>::holder column_;
  };

  template<typename base_builder, typename column_type, typename... column_types>
  class _database_order_builder<base_builder, column_type, column_types...>
    : public _database_order_builder<base_builder, column_types...>
  {
    using base_class = _database_order_builder<base_builder, column_types...>;
    using this_class = _database_order_builder<base_builder, column_type, column_types...>;
  public:
    _database_order_builder(
      base_builder && b,
      column_type && column,
      column_types &&... columns)
      : base_class(std::move(b), std::forward<column_types>(columns)...),
        column_(std::forward<column_type>(column))
    {
    }
    
    std::string final_sql(_database_sql_builder & builder) const
    {
      return " ORDER BY " + this->generate_columns(builder);
    }
    
    std::string generate_columns(_database_sql_builder & builder) const
    {
      return this->column_.visit(builder) + "," + base_builder::generate_columns(builder);
    }

  private:
    typename _database_order_holder<column_type>::holder column_;
  };
  /////////////////////////
  template <typename base_builder, typename condition_type>
  class _database_reader_builder_with_where : public base_builder
  {
    using this_class = _database_reader_builder_with_where<base_builder, condition_type>;
  public:
    _database_reader_builder_with_where(
      base_builder && b,
      condition_type && cond)
      : base_builder(std::move(b)),
      cond_(std::move(cond))
    {
    }

    std::string collect_condition(_database_sql_builder & builder) const
    {
      return " WHERE " + this->cond_.visit(builder);
    }
    
    template <typename... order_columns_types>
    _database_order_builder<this_class, order_columns_types...> order_by(order_columns_types && ... order_columns)
    {
      return _database_order_builder<this_class, order_columns_types...>(
        std::move(*this),
        std::forward<order_columns_types>(order_columns)...);
    }
    

  private:
    condition_type cond_;
  };

  template <typename base_builder, typename condition_type>
  class _database_reader_builder_with_join : public base_builder
  {
    using this_class = _database_reader_builder_with_join;
  public:
    _database_reader_builder_with_join(
      base_builder && b,
      const database_table * table,
      condition_type && cond)
      : base_builder(std::move(b)),
        table_(table),
        cond_(std::move(cond))
    {
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> inner_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond));
    }

    template <typename where_condition_type>
    _database_reader_builder_with_where<this_class, where_condition_type> where(where_condition_type && cond)
    {
      return _database_reader_builder_with_where<this_class, where_condition_type>(std::move(*this), std::move(cond));
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      base_builder::collect_aliases(aliases);

      aliases[this->table_] = "t" + std::to_string(aliases.size());
    }

    std::string collect_sources(_database_sql_builder & builder) const
    {
      return base_builder::collect_sources(builder)
        + " INNER JOIN " + this->table_->name() + " " + builder.get_alias(this->table_)
        + " ON " + this->cond_.visit(builder);
    }

  private:
    const database_table * table_;
    condition_type cond_;
  };

  template<typename column_type, typename dummy = void>
  class _database_column_holder;

  template<typename column_type>
  class _database_column_holder<column_type, typename std::enable_if<std::is_base_of<_database_column_base, typename std::remove_reference<column_type>::type>::value>::type>
  {
  public:
    using holder = _db_simple_column;
  };

  template<typename column_type>
  class _database_column_holder<column_type, typename std::enable_if<!std::is_base_of<_database_column_base, typename std::remove_reference<column_type>::type>::value>::type>
  {
  public:
    using holder = column_type;
  };
  
  template<typename base_builder, typename column_type>
  class _database_select_builder<base_builder, column_type> : public base_builder
  {
    using this_class = _database_select_builder<base_builder, column_type>;
  public:
    _database_select_builder(
      base_builder && b,
      database_table * t,
      column_type && column)
      : base_builder(std::move(b)),
        t_(t),
        column_(std::forward<column_type>(column))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      aliases[this->t_] = "t0";
    }
    
    std::string start_sql(_database_sql_builder & builder) const
    {
      return base_builder::start_sql(builder) + "SELECT ";
    }
    
    std::string collect_sources(_database_sql_builder & builder) const
    {
      return " FROM " + this->t_->name() + " " + builder.get_alias(this->t_);
    }
    
    std::string generate_select(_database_sql_builder & builder) const
    {
      return this->column_.visit(builder);
    }
    
    template <typename where_condition_type>
    _database_reader_builder_with_where<this_class, where_condition_type> where(where_condition_type && cond)
    {
      return _database_reader_builder_with_where<this_class, where_condition_type>(std::move(*this), std::move(cond));
    }

  protected:
    database_table * t_;
    typename _database_column_holder<column_type>::holder column_;
  };

  template<typename base_builder, typename column_type, typename... column_types>
  class _database_select_builder<base_builder, column_type, column_types...>
    : public _database_select_builder<base_builder, column_types...>
  {
    using base_class = _database_select_builder<base_builder, column_types...>;
    using this_class = _database_select_builder<base_builder, column_type, column_types...>;
  public:
    _database_select_builder(
      base_builder && b,
      database_table * t,
      column_type && column,
      column_types &&... columns)
      : base_class(std::move(b), t, std::forward<column_types>(columns)...),
        column_(std::forward<column_type>(column))
    {
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> inner_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond));
    }

    std::string generate_select(_database_sql_builder & builder) const
    {
      return this->column_.visit(builder) + "," + base_class::generate_select(builder);
    }
    
    template <typename where_condition_type>
    _database_reader_builder_with_where<this_class, where_condition_type> where(where_condition_type && cond)
    {
      return _database_reader_builder_with_where<this_class, where_condition_type>(std::move(*this), std::move(cond));
    }

  private:
    typename _database_column_holder<column_type>::holder column_;
  };
    

  inline const std::string & _database_sql_builder::get_alias(const database_table * t) const
  {
    auto p = this->aliases_.find(t);
    if(this->aliases_.end() == p){
      throw std::runtime_error("Table " + t->name() + " not found");
    }
    
    return p->second;
  }
  
  /////////////////////////////// Insert ////////////////////////
  class _database_insert_builder : public _database_source_base
  {
  public:
    
    template<typename... setter_types>
    _database_insert_builder(const database_table & table, setter_types &&... setters)
    : table_(table)
    {
      this->set(std::forward<setter_types>(setters)...);
    }
    
    template<typename... setter_types>
    _database_insert_builder & set(_database_column_setter && setter, setter_types &&... setters)
    {
      return this->set(std::move(setter)).set(std::move(setters)...);
    }
   
    _database_insert_builder & set(_database_column_setter && setter)
    {
      this->columns_.push_back(setter.column());
      this->values_.push_back(setter.set_parameter());
      return *this;
    }
    
    std::string start_sql(_database_sql_builder & builder) const
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
      
      auto p = this->values_.begin();
      for(size_t index = 0; index < this->columns_.size(); ++index){
        if(0 < index){
          sql += ",";
        }
        sql += builder.add_parameter(*p++);
      }
      sql += ")";
      
      return sql;      
    }
    
  private:
    const database_table & table_;
    std::list<const _database_column_base *> columns_;
    std::list<std::function<void(sql_statement & st, int index)>> values_;
  };
  
  /////////////////////////////// Insert from ////////////////////////
  class _database_insert_from_builder : public _database_source_base
  {
  public:
    
    template<typename... column_types>
    _database_insert_from_builder(const database_table * table, column_types &... columns)
    : table_(table)
    {
      this->set(columns...);
    }
   
    std::string start_sql(_database_sql_builder & builder) const
    {
      std::string sql = "INSERT INTO " + this->table_->name() + "(";
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
      
      return sql;
    }
    
    template<typename... column_types>
    _database_select_builder<_database_insert_from_builder, column_types...> from(
      database_table & t,
      column_types &&... columns)
    {
      return _database_select_builder<_database_insert_from_builder, column_types...>(
        std::move(*this),
        &t,
        std::forward<column_types>(columns)...);
    }
    
  private:
    const database_table * table_;
    std::list<const _database_column_base *> columns_;
    
    template<typename... column_types>
    _database_insert_from_builder & set(_database_column_base & column, column_types &&... columns)
    {
      return this->set(column).set(std::forward<column_types>(columns)...);
    }
   
    _database_insert_from_builder & set(_database_column_base & column)
    {
      this->columns_.push_back(&column);
      return *this;
    }
  };

  /////////////////////////////// Delete ////////////////////////
  template <typename condition_type>
  class _database_delete_builder : public _database_source_base
  {
  public:
    _database_delete_builder(
      const database_table & table,
      condition_type && cond)
      : table_(table),
        cond_(std::move(cond))
    {
    }
    
    std::string start_sql(_database_sql_builder & builder) const
    {
      return "DELETE FROM " + this->table_.name();
    }
    
    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      aliases[&this->table_] = this->table_.name();
    }
   
    std::string collect_condition(_database_sql_builder & builder) const
    {
      return " WHERE " + this->cond_.visit(builder);
    }
    
  private:
    const database_table & table_;
    condition_type cond_;
  };
  
  template<typename implementation_type, typename left_exp_type, typename right_exp_type>
  template<typename other_exp>
  inline _database_logical_and<implementation_type, other_exp> _database_binary_expression<implementation_type, left_exp_type, right_exp_type>::operator&&(other_exp && exp)
  {
    return _database_logical_and<implementation_type, other_exp>(std::move(*static_cast<implementation_type *>(this)), std::move(exp));
  }

  template<typename... setter_types>
  inline _database_insert_builder database_table::insert(setter_types &&... setters)
  {
    return _database_insert_builder(*this, std::forward<setter_types>(setters)...);
  }
  
  template <typename source_type>
  class sql_command_builder
  {
  public:
    sql_statement build(database_transaction & t, const source_type & source)
    {
      std::map<const database_table *, std::string> aliases;
      source.collect_aliases(aliases);

      _database_sql_builder builder(aliases);
      auto sql =
        source.start_sql(builder)
        + source.generate_select(builder)
        + source.collect_sources(builder)
        + source.collect_condition(builder)
        + source.final_sql(builder);

      return builder.set_parameters(t.parse(sql.c_str()));
    }
  };
  
  /////////////////////// implementation database_table //////////////////////
  template<typename... column_types>
  inline _database_select_builder<_database_source_base, column_types...> database_table::select(
    column_types &&... columns)
  {
    return _database_select_builder<_database_source_base, column_types...>(
      _database_source_base(),
      this,
      std::forward<column_types>(columns)...);
  }
  
  template<typename... column_types>
  _database_insert_from_builder database_table::insert_into(column_types &&... columns)
  {
    return _database_insert_from_builder(this, std::forward<column_types>(columns)...);
  }
  
  template<typename expression_type>
  _database_delete_builder<expression_type> database_table::delete_if(expression_type && cond)
  {
    return _database_delete_builder<expression_type>(*this, std::move(cond));
  }
  
  //////////////////////// database_column /////////////////////////////
  template <typename value_type, typename db_value_type>
  inline value_type database_column<value_type, db_value_type>::get(sql_statement & st) const {
    db_value_type result;
    st.get_value(this->index_, result);
    return (value_type)result;
  }

  template <typename value_type, typename db_value_type>
  inline _database_expression_equ_exp<_database_column_exp, _database_value_exp<value_type>> database_column<value_type, db_value_type>::operator == (value_type value) const {
    return _database_expression_equ_exp<_database_column_exp, _database_value_exp<value_type>>(
        _database_column_exp(this),
        _database_value_exp<value_type>(value));
  }
    
  template <typename value_type, typename db_value_type>
  inline _database_expression_equ_exp<_database_column_exp, _database_column_exp> database_column<value_type, db_value_type>::operator == (const database_column<value_type> & right) const {
    return _database_expression_equ_exp<_database_column_exp, _database_column_exp>(
        _database_column_exp(this),
        _database_column_exp(&right));
  }
    
  template <typename value_type, typename db_value_type>
  _database_column_setter database_column<value_type, db_value_type>::operator = (const value_type & value) const {
    return _database_column_setter(this, [val = (db_value_type)value](sql_statement & st, int index) {
      st.set_parameter(index, val);
    });
  }
  
}

#endif // __VDS_DATABASE_DATABASE_ORM_H_
