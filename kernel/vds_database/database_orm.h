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
  
  class _database_update_builder;
  
  template<typename expression_type>
  class _database_delete_builder;

  class _database_column_setter;  
  class _database_column_exp;
  
  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_equ_exp;

  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_not_equ_exp;

  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_less_or_equ_exp;

  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_greater_exp;

  template<typename value_type>
  class _database_value_exp;
  
  class _database_source_base;

  template<typename left_exp_type, typename right_exp_type>
  class _database_logical_or;

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

    template<typename... setter_types>
    _database_insert_builder insert_or_ignore(setter_types &&... setters);

    template<typename... setter_types>
    _database_update_builder update(setter_types &&... setters);
    
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
      : owner_(owner), name_(name), index_(-1)
    {
    }

    const database_table * owner() const { return this->owner_; }
    const std::string & name() const { return this->name_; }
    
    void set_index(int index)
    {
      this->index_ = index;
    }

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

    _database_expression_not_equ_exp<_database_column_exp, _database_value_exp<value_type>> operator != (value_type value) const;
    _database_expression_not_equ_exp<_database_column_exp, _database_column_exp> operator != (const database_column<value_type> & right) const;

    _database_expression_less_or_equ_exp<_database_column_exp, _database_value_exp<value_type>> operator <= (value_type value) const;

    _database_expression_greater_exp<_database_column_exp, _database_value_exp<value_type>> operator > (value_type value) const;

    _database_column_setter operator = (const value_type & value) const;

    bool is_null(const sql_statement &statement) const;
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
      return "?" + std::to_string(this->set_parameters_.size());
    }
    
    sql_statement set_parameters(sql_statement && st)
    {
      int index = 0;
      for(auto & p : this->set_parameters_){
        p(st, ++index);
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

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
    }

    std::string visit(_database_sql_builder & builder) const
    {
      auto alias = builder.get_alias(this->column_->owner());
      if (alias.empty()) {
        return this->column_->name();
      }
      else {
        return alias + "." + this->column_->name();
      }
    }
    
  private:
    const _database_column_base * column_;
  };

  class _db_simple_column
  {
  public:
    _db_simple_column(
      _database_column_base & column)
      : column_(&column)
    {
    }
    
    _db_simple_column(
      _db_simple_column && column)
      : column_(column.column_)
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
    }

    std::string visit(_database_sql_builder & builder) const
    {
      auto alias = builder.get_alias(this->column_->owner());
      if (alias.empty()) {
        return this->column_->name();
      }
      else {
        return alias + "." + this->column_->name();
      }
    }

    void set_index(int index) const
    {
      this->column_->set_index(index);
    }

  private:
    _database_column_base * column_;
  };

  template <typename source_type>
  class _db_min
  {
  public:
    _db_min(
      source_type && column)
      : column_(std::move(column))
    {

    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->column_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return "MIN(" + this->column_.visit(builder) + ")";
    }

    void set_index(int index) const
    {
    }

  private:
    source_type column_;
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

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->column_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return "MAX(" + this->column_.visit(builder) + ")";
    }

    void set_index(int index) const
    {
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

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->column_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return "LENGTH(" + this->column_.visit(builder) + ")";
    }

    void set_index(int index) const
    {
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

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->column_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return "SUM(" + this->column_.visit(builder) + ")";
    }

    void set_index(int index) const
    {
    }

  private:
    source_type column_;
  };
  
  template <typename source_type>
  class _db_count
  {
  public:
    _db_count(
      source_type && column)
      : column_(std::move(column))
    {

    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->column_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return "COUNT(" + this->column_.visit(builder) + ")";
    }

    void set_index(int index) const
    {
    }

  private:
    source_type column_;
  };

  template <typename source_type, typename dummy = typename std::enable_if<std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_min<_db_simple_column> db_min(source_type & column)
  {
    return _db_min<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<!std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_min<source_type> db_min(source_type && column)
  {
    return _db_min<source_type>(std::move(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_max<_db_simple_column> db_max(source_type & column)
  {
    return _db_max<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<!std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_max<source_type> db_max(source_type && column)
  {
    return _db_max<source_type>(std::move(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_length<_db_simple_column> db_length(source_type & column)
  {
    return _db_length<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<!std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_length<source_type> db_length(source_type && column)
  {
    return _db_length<source_type>(std::move(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_sum<_db_simple_column> db_sum(source_type & column)
  {
    return _db_sum<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<!std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_sum<source_type> db_sum(source_type && column)
  {
    return _db_sum<source_type>(std::move(column));
  }
  
  template <typename source_type, typename dummy = typename std::enable_if<std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_count<_db_simple_column> db_count(source_type & column)
  {
    return _db_count<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = typename std::enable_if<!std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>::type>
  inline _db_count<source_type> db_count(source_type && column)
  {
    return _db_count<source_type>(std::move(column));
  }

  template <typename source_type>
  class _db_order_desc
  {
  public:
    _db_order_desc(
      source_type && column)
      : column_(std::move(column))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->column_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return this->column_.visit(builder) + " DESC";
    }

  private:
    source_type column_;
  };

  template <typename source_type, typename dummy = std::enable_if_t<std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>>
  inline _db_order_desc<_db_simple_column> db_desc_order(source_type & column)
  {
    return _db_order_desc<_db_simple_column>(_db_simple_column(column));
  }

  template <typename source_type, typename dummy = std::enable_if_t<!std::is_base_of<_database_column_base, typename std::remove_reference<source_type>::type>::value>>
  inline _db_order_desc<source_type> db_desc_order(source_type && column)
  {
    return _db_order_desc<source_type>(std::move(column));
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

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
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

    template <typename other_exp>
    _database_logical_or<implementation_type, other_exp> operator || (other_exp && exp);
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

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->left_.collect_aliases(aliases);
      this->right_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return this->left_.visit(builder) + "=" + this->right_.visit(builder);
    }
  };

  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_not_equ_exp : public _database_binary_expression<_database_expression_not_equ_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>
  {
    using base_class = _database_binary_expression<_database_expression_not_equ_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>;
  public:
    _database_expression_not_equ_exp(
      left_exp_type && left,
      right_exp_type && right)
      : base_class(std::move(left), std::move(right))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const {
      this->left_.collect_aliases(aliases);
      this->right_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return this->left_.visit(builder) + "<>" + this->right_.visit(builder);
    }
  };

  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_less_or_equ_exp : public _database_binary_expression<_database_expression_less_or_equ_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>
  {
    using base_class = _database_binary_expression<_database_expression_equ_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>;
  public:
    _database_expression_less_or_equ_exp(
      left_exp_type && left,
      right_exp_type && right)
      : base_class(std::move(left), std::move(right))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->left_.collect_aliases(aliases);
      this->right_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return this->left_.visit(builder) + "<=" + this->right_.visit(builder);
    }
  };

  template<typename left_exp_type, typename right_exp_type>
  class _database_expression_greater_exp : public _database_binary_expression<_database_expression_greater_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>
  {
    using base_class = _database_binary_expression<_database_expression_greater_exp<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>;
  public:
    _database_expression_greater_exp(
      left_exp_type && left,
      right_exp_type && right)
      : base_class(std::move(left), std::move(right))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const {
      this->left_.collect_aliases(aliases);
      this->right_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return this->left_.visit(builder) + ">" + this->right_.visit(builder);
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

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->left_.collect_aliases(aliases);
      this->right_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return "(" + this->left_.visit(builder) + ") AND (" + this->right_.visit(builder) + ")";
    }
  };

  template<typename left_exp_type, typename right_exp_type>
  class _database_logical_or : public _database_binary_expression<_database_logical_or<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>
  {
    using base_class = _database_binary_expression<_database_logical_or<left_exp_type, right_exp_type>, left_exp_type, right_exp_type>;
  public:
    _database_logical_or(
        left_exp_type && left,
        right_exp_type && right)
        : base_class(std::move(left), std::move(right))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const{
      this->left_.collect_aliases(aliases);
      this->right_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return "(" + this->left_.visit(builder) + ") OR (" + this->right_.visit(builder) + ")";
    }
  };

  template <typename source_type, typename command_type>
  class _db_not_in{
  public:
    _db_not_in(
        source_type && left,
        command_type && right)
        : left_(std::move(left)),
          right_(std::move(right)){
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const {
      this->left_.collect_aliases(aliases);
      this->right_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      return this->left_.visit(builder)
             + " NOT IN ("
             + this->right_.start_sql(builder)
             + this->right_.generate_select(builder, -5000)//To fail if get
             + this->right_.collect_sources(builder)
             + this->right_.collect_condition(builder)
             + this->right_.final_sql(builder)
             + ")";
    }

  private:
    source_type left_;
    command_type right_;
  };

  template <typename source_type, typename container_type>
  class _db_not_in_values{
  public:
    _db_not_in_values(
        source_type && left,
        const container_type & right)
        : left_(std::move(left)),
          right_(right){
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const {
      this->left_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      std::string postfix;
      for(auto p : this->right_){
        if(!postfix.empty()){
          postfix += ',';
        }

        postfix += builder.add_parameter([p](sql_statement & st, int index){
          st.set_parameter(index, p);
        });
      }

      return this->left_.visit(builder)
             + " NOT IN (" + postfix + ")";
    }

  private:
    source_type left_;
    container_type right_;
  };

  template <typename source_type, typename container_type>
  class _db_in_values{
  public:
    _db_in_values(
        source_type && left,
        const container_type & right)
        : left_(std::move(left)),
          right_(right){
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const {
      this->left_.collect_aliases(aliases);
    }

    std::string visit(_database_sql_builder & builder) const
    {
      std::string postfix;
      for(auto p : this->right_){
        if(!postfix.empty()){
          postfix += ',';
        }

        postfix += builder.add_parameter([p](sql_statement & st, int index){
          st.set_parameter(index, p);
        });
      }

      return this->left_.visit(builder)
             + " IN (" + postfix + ")";
    }

  private:
    source_type left_;
    container_type right_;
  };

  template <typename source_type, typename command_type>
  inline _db_not_in<_db_simple_column, command_type> db_not_in(source_type & column, command_type && command)
  {
    return _db_not_in<_db_simple_column, command_type>(_db_simple_column(column), std::move(command));
  }

  template <typename source_type, typename container_type>
  inline _db_not_in_values<_db_simple_column, container_type> db_not_in_values(source_type & column, const container_type & values)
  {
    return _db_not_in_values<_db_simple_column, container_type>(_db_simple_column(column), values);
  }

  template <typename source_type, typename container_type>
  inline _db_in_values<_db_simple_column, container_type> db_in_values(source_type & column, const container_type & values)
  {
    return _db_in_values<_db_simple_column, container_type>(_db_simple_column(column), values);
  }

  template <typename base_builder, typename condition_type>
  class _database_reader_builder_with_join;

  template <typename base_builder, typename condition_type>
  class _database_reader_builder_with_where;
  
  class _database_source_base
  {
  public:
    void collect_aliases(std::map<const database_table *, std::string> & /*aliases*/) const
    {
    }
    
    std::string start_sql(_database_sql_builder & /*builder*/) const
    {
      return std::string();
    }
    
    std::string generate_select(_database_sql_builder & /*builder*/, int /*index*/) const
    {
      return std::string();
    }
    
    std::string collect_sources(_database_sql_builder & /*builder*/) const
    {
      return std::string();
    }
    
    std::string collect_condition(_database_sql_builder & /*builder*/) const
    {
      return std::string();
    }
    
    std::string final_sql(_database_sql_builder & /*builder*/) const
    {
      return std::string();
    }
  };
  template <typename base_source_type>
  class _database_source_impl
  {
  public:
    _database_source_impl(base_source_type && base_source)
    : base_source_(std::move(base_source)){
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      this->base_source_.collect_aliases(aliases);
    }

    std::string start_sql(_database_sql_builder & builder) const
    {
      return this->base_source_.start_sql(builder);
    }

    std::string generate_select(_database_sql_builder & builder, int index) const
    {
      return this->base_source_.generate_select(builder, index);
    }

    std::string collect_sources(_database_sql_builder & builder) const
    {
      return this->base_source_.collect_sources(builder);
    }

    std::string collect_condition(_database_sql_builder & builder) const
    {
      return this->base_source_.collect_condition(builder);
    }

    std::string final_sql(_database_sql_builder & builder) const
    {
      return this->base_source_.final_sql(builder);
    }

  private:
    base_source_type base_source_;
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
  class _database_order_builder<base_builder, column_type> : public _database_source_impl<base_builder>
  {
    using this_class = _database_order_builder<base_builder, column_type>;
  public:
    _database_order_builder(
      base_builder && b,
      column_type && column)
      : _database_source_impl<base_builder>(std::move(b)),
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
      return this->column_.visit(builder) + "," + base_class::generate_columns(builder);
    }

  private:
    typename _database_order_holder<column_type>::holder column_;
  };
  /////////////////////////
  class _db_group_by_column
  {
  public:
    _db_group_by_column(const _database_column_base & column)
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
  class _database_group_by_builder;
  
  template<typename column_type, typename dummy = void>
  class _database_group_by_holder;

  template<typename column_type>
  class _database_group_by_holder<column_type, typename std::enable_if<std::is_base_of<_database_column_base, typename std::remove_reference<column_type>::type>::value>::type>
  {
  public:
    using holder = _db_group_by_column;
  };

  template<typename column_type>
  class _database_group_by_holder<column_type, typename std::enable_if<!std::is_base_of<_database_column_base, typename std::remove_reference<column_type>::type>::value>::type>
  {
  public:
    using holder = column_type;
  };

  template<typename base_builder, typename column_type>
  class _database_group_by_builder<base_builder, column_type> : public _database_source_impl<base_builder>
  {
    using this_class = _database_group_by_builder<base_builder, column_type>;
  public:
    _database_group_by_builder(
      base_builder && b,
      column_type && column)
      : _database_source_impl<base_builder>(std::move(b)),
        column_(std::forward<column_type>(column))
    {
    }

    std::string final_sql(_database_sql_builder & builder) const
    {
      return " GROUP BY " + this->generate_columns(builder);
    }

    std::string generate_columns(_database_sql_builder & builder) const
    {
      return this->column_.visit(builder);
    }

  protected:
    typename _database_group_by_holder<column_type>::holder column_;
  };

  template<typename base_builder, typename column_type, typename... column_types>
  class _database_group_by_builder<base_builder, column_type, column_types...>
    : public _database_group_by_builder<base_builder, column_types...>
  {
    using base_class = _database_group_by_builder<base_builder, column_types...>;
    using this_class = _database_group_by_builder<base_builder, column_type, column_types...>;
  public:
    _database_group_by_builder(
      base_builder && b,
      column_type && column,
      column_types &&... columns)
      : base_class(std::move(b), std::forward<column_types>(columns)...),
        column_(std::forward<column_type>(column))
    {
    }
    
    std::string final_sql(_database_sql_builder & builder) const
    {
      return " GROUP BY " + this->generate_columns(builder);
    }
    
    std::string generate_columns(_database_sql_builder & builder) const
    {
      return this->column_.visit(builder) + "," + base_class::generate_columns(builder);
    }

  private:
    typename _database_group_by_holder<column_type>::holder column_;
  };
  /////////////////////////
  template <typename base_builder, typename condition_type>
  class _database_builder_with_where : public _database_source_impl<base_builder>
  {
    using base_class = _database_source_impl<base_builder>;
    using this_class = _database_builder_with_where<base_builder, condition_type>;
  public:
    _database_builder_with_where(
      base_builder && b,
      condition_type && cond)
      : base_class(std::move(b)),
      cond_(std::move(cond))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const {
      base_class::collect_aliases(aliases);

      this->cond_.collect_aliases(aliases);
    }

    std::string collect_condition(_database_sql_builder & builder) const
    {
      return " WHERE " + this->cond_.visit(builder);
    }
    
  private:
    condition_type cond_;
  };
  /////////////////////////
  template <typename base_builder, typename condition_type>
  class _database_reader_builder_with_where : public _database_builder_with_where<base_builder, condition_type>
  {
    using base_class = _database_builder_with_where<base_builder, condition_type>;
    using this_class = _database_reader_builder_with_where<base_builder, condition_type>;
  public:
    _database_reader_builder_with_where(
      base_builder && b,
      condition_type && cond)
      : base_class(std::move(b), std::move(cond))
    {
    }
   
    template <typename... order_columns_types>
    _database_order_builder<this_class, order_columns_types...> order_by(order_columns_types && ... order_columns)
    {
      return _database_order_builder<this_class, order_columns_types...>(
        std::move(*this),
        std::forward<order_columns_types>(order_columns)...);
    }
  };

  enum join_t
  {
    inner,
    left,
    right,
    outer
  };

  template <typename base_builder, typename condition_type>
  class _database_reader_builder_with_join : public _database_source_impl<base_builder>
  {
    using this_class = _database_reader_builder_with_join;
  public:
    _database_reader_builder_with_join(
      base_builder && b,
      const database_table * table,
      condition_type && cond,
      join_t join_type)
      : _database_source_impl<base_builder>(std::move(b)),
        table_(table),
        cond_(std::move(cond)),
        join_type_(join_type)
    {
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> inner_join(
        const database_table & t,
        join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(
          std::move(*this), &t, std::move(cond), join_t::inner);
    }
    
    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> left_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::left);
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> right_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::right);
    }

    template <typename where_condition_type>
    _database_reader_builder_with_where<this_class, where_condition_type> where(
        where_condition_type && cond)
    {
      return _database_reader_builder_with_where<this_class, where_condition_type>(
          std::move(*this), std::move(cond));
    }

    template <typename... group_by_columns_types>
    _database_group_by_builder<this_class, group_by_columns_types...> group_by(group_by_columns_types && ... group_by_columns)
    {
      return _database_group_by_builder<this_class, group_by_columns_types...>(
        std::move(*this),
        std::forward<group_by_columns_types>(group_by_columns)...);
    }

    template <typename... order_columns_types>
    _database_order_builder<this_class, order_columns_types...> order_by(
        order_columns_types && ... order_columns)
    {
      return _database_order_builder<this_class, order_columns_types...>(
          std::move(*this),
          std::forward<order_columns_types>(order_columns)...);
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      _database_source_impl<base_builder>::collect_aliases(aliases);

      aliases[this->table_] = "t" + std::to_string(aliases.size());
    }

    std::string collect_sources(_database_sql_builder & builder) const
    {
      const char * join_str;
      switch (this->join_type_){
        case join_t::inner:
          join_str = " INNER JOIN ";
          break;
        case join_t::left:
          join_str = " LEFT JOIN ";
          break;
        case join_t::right:
          join_str = " RIGHT JOIN ";
          break;
        case join_t::outer:
          join_str = " OUTER JOIN ";
          break;
        default:
          throw std::runtime_error("Invalid data");
      }

      return _database_source_impl<base_builder>::collect_sources(builder)
        + join_str
        + this->table_->name() + " " + builder.get_alias(this->table_)
        + " ON " + this->cond_.visit(builder);
    }

  private:
    const database_table * table_;
    condition_type cond_;
    join_t join_type_;
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
  class _database_select_builder<base_builder, column_type> : public _database_source_impl<base_builder>
  {
    using this_class = _database_select_builder<base_builder, column_type>;
  public:
    _database_select_builder(
      base_builder && b,
      database_table * t,
      column_type && column)
      : _database_source_impl<base_builder>(std::move(b)),
        t_(t),
        column_(std::forward<column_type>(column))
    {
    }

    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      _database_source_impl<base_builder>::collect_aliases(aliases);

      aliases[this->t_] = "t" + std::to_string(aliases.size());
    }
    
    std::string start_sql(_database_sql_builder & builder) const
    {
      return _database_source_impl<base_builder>::start_sql(builder) + "SELECT ";
    }
    
    std::string collect_sources(_database_sql_builder & builder) const
    {
      return " FROM " + this->t_->name() + " " + builder.get_alias(this->t_);
    }
    
    std::string generate_select(_database_sql_builder & builder, int index) const
    {
      this->column_.set_index(index);
      return this->column_.visit(builder);
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> inner_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::inner);
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> left_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::left);
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> right_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::right);
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> outer_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::outer);
    }

    template <typename where_condition_type>
    _database_reader_builder_with_where<this_class, where_condition_type> where(where_condition_type && cond)
    {
      return _database_reader_builder_with_where<this_class, where_condition_type>(std::move(*this), std::move(cond));
    }

    template <typename... group_by_columns_types>
    _database_group_by_builder<this_class, group_by_columns_types...> group_by(group_by_columns_types && ... group_by_columns)
    {
      return _database_group_by_builder<this_class, group_by_columns_types...>(
        std::move(*this),
        std::forward<group_by_columns_types>(group_by_columns)...);
    }

    template <typename... order_columns_types>
    _database_order_builder<this_class, order_columns_types...> order_by(order_columns_types && ... order_columns)
    {
      return _database_order_builder<this_class, order_columns_types...>(
        std::move(*this),
        std::forward<order_columns_types>(order_columns)...);
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
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::inner);
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> left_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::left);
    }

    template <typename join_condition_type>
    _database_reader_builder_with_join<this_class, join_condition_type> right_join(const database_table & t, join_condition_type && cond)
    {
      return _database_reader_builder_with_join<this_class, join_condition_type>(std::move(*this), &t, std::move(cond), join_t::right);
    }

    std::string generate_select(_database_sql_builder & builder, int index) const
    {
      this->column_.set_index(index);
      return this->column_.visit(builder) + "," + base_class::generate_select(builder, index + 1);
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
    enum class Insert_Strategy
    {
      Regular,
      Replace,
      Ignore
    };
    
    template<typename... setter_types>
    _database_insert_builder(const database_table & table, setter_types &&... setters)
    : strategy_(Insert_Strategy::Regular), table_(table)
    {
      this->set(std::forward<setter_types>(setters)...);
    }

    template<typename... setter_types>
    _database_insert_builder(Insert_Strategy strategy, const database_table & table, setter_types &&... setters)
      : strategy_(strategy), table_(table)
    {
      this->set(std::forward<setter_types>(setters)...);
    }

    std::string start_sql(_database_sql_builder & builder) const
    {
      std::string sql;
      switch (this->strategy_) {
      case Insert_Strategy::Replace:
        sql = "REPLACE";
        break;

      case Insert_Strategy::Ignore:
        sql = "INSERT OR IGNORE";
        break;

      default:
        sql = "INSERT";
        break;
      }
      sql += " INTO " + this->table_.name() + "(";
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
    Insert_Strategy strategy_;
    const database_table & table_;
    std::list<const _database_column_base *> columns_;
    std::list<std::function<void(sql_statement & st, int index)>> values_;
    
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
  };
  
  /////////////////////////////// Update ////////////////////////
  class _database_update_builder : public _database_source_base
  {
    using this_class = _database_update_builder;
  public:
    
    template<typename... setter_types>
    _database_update_builder(const database_table & table, setter_types &&... setters)
    : table_(table)
    {
      this->set(std::forward<setter_types>(setters)...);
    }
    
    void collect_aliases(std::map<const database_table *, std::string> & aliases) const
    {
      aliases[&this->table_] = std::string();
    }

    
    std::string start_sql(_database_sql_builder & builder) const
    {
      std::string sql = "UPDATE " + this->table_.name() + " SET ";
      
      bool is_first = true;
      auto p = this->values_.begin();
      for(auto column : this->columns_){
        if(is_first){
          is_first = false;
        }
        else {
          sql += ",";
        }
        sql += column->name();
        sql += "=";
        sql += builder.add_parameter(*p++);
      }
      
      return sql;      
    }
    
    template <typename where_condition_type>
    _database_builder_with_where<this_class, where_condition_type> where(where_condition_type && cond)
    {
      return _database_builder_with_where<this_class, where_condition_type>(std::move(*this), std::move(cond));
    }
    
  private:
    const database_table & table_;
    std::list<const _database_column_base *> columns_;
    std::list<std::function<void(sql_statement & st, int index)>> values_;
    
    template<typename... setter_types>
    _database_update_builder & set(_database_column_setter && setter, setter_types &&... setters)
    {
      return this->set(std::move(setter)).set(std::move(setters)...);
    }
   
    _database_update_builder & set(_database_column_setter && setter)
    {
      this->columns_.push_back(setter.column());
      this->values_.push_back(setter.set_parameter());
      return *this;
    }
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
   
    std::string start_sql(_database_sql_builder & /*builder*/) const
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
      this->cond_.collect_aliases(aliases);
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

  template<typename implementation_type, typename left_exp_type, typename right_exp_type>
  template<typename other_exp>
  inline _database_logical_or<implementation_type, other_exp> _database_binary_expression<implementation_type, left_exp_type, right_exp_type>::operator||(other_exp && exp)
  {
    return _database_logical_or<implementation_type, other_exp>(std::move(*static_cast<implementation_type *>(this)), std::move(exp));
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
        + source.generate_select(builder, 0)
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
  
  template<typename... setter_types>
  inline _database_insert_builder database_table::insert(setter_types &&... setters)
  {
    return _database_insert_builder(*this, std::forward<setter_types>(setters)...);
  }

  template<typename... setter_types>
  inline _database_insert_builder database_table::insert_or_ignore(setter_types &&... setters)
  {
    return _database_insert_builder(_database_insert_builder::Insert_Strategy::Ignore, *this, std::forward<setter_types>(setters)...);
  }

  template<typename... column_types>
  _database_insert_from_builder database_table::insert_into(column_types &&... columns)
  {
    return _database_insert_from_builder(this, std::forward<column_types>(columns)...);
  }
  
  template<typename... setter_types>
  inline _database_update_builder database_table::update(setter_types &&... setters)
  {
    return _database_update_builder(*this, std::forward<setter_types>(setters)...);
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
  inline bool database_column<value_type, db_value_type>::is_null(
      const sql_statement &statement) const{
    return statement.is_null(this->index_);
  }

  template <typename value_type, typename db_value_type>
  inline _database_expression_less_or_equ_exp<_database_column_exp, _database_value_exp<value_type>> database_column<value_type, db_value_type>::operator <= (value_type value) const {
    return _database_expression_less_or_equ_exp<_database_column_exp, _database_value_exp<value_type>>(
      _database_column_exp(this),
      _database_value_exp<value_type>(value));
  }

  template <typename value_type, typename db_value_type>
  _database_expression_greater_exp<_database_column_exp, _database_value_exp<value_type>> database_column<value_type,
  db_value_type>::operator > (value_type value) const {
    return _database_expression_greater_exp<_database_column_exp, _database_value_exp<value_type>>(
      _database_column_exp(this),
      _database_value_exp<value_type>(value));
  }

  template <typename value_type, typename db_value_type>
  _database_column_setter database_column<value_type, db_value_type>::operator = (const value_type & value) const {
    return _database_column_setter(this, [val = (db_value_type)value](sql_statement & st, int index) {
      st.set_parameter(index, val);
    });
  }
  
}

#endif // __VDS_DATABASE_DATABASE_ORM_H_
