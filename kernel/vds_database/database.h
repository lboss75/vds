#ifndef __VDS_DATABASE_DATABASE_H_
#define __VDS_DATABASE_DATABASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _database;
  class _sql_statement;

  class sql_statement
  {
  public:
    sql_statement(_sql_statement * impl);
    sql_statement(sql_statement && original);
    ~sql_statement();

    void set_parameter(int index, uint64_t value);
    void set_parameter(int index, const std::string & value);
    void set_parameter(int index, const guid & value);
    void set_parameter(int index, const data_buffer & value);

    template <typename... argument_types>
    void set_parameters(argument_types... arguments)
    {
      this->set_parameters_(1, arguments...);
    }
    
    bool execute();

    bool get_value(int index, uint64_t & value);
    bool get_value(int index, std::string & value);
    bool get_value(int index, guid & value);
    bool get_value(int index, data_buffer & value);
    

  private:
    _sql_statement * impl_;
    
    template <typename first_argument_type, typename... argument_types>
    void set_parameters_(int index, first_argument_type first_argument, argument_types... arguments)
    {
      this->set_parameter(index, first_argument);
      this->set_parameters_(index + 1, arguments...);
    }
    
    void set_parameters_(int index)
    {
    }
    
  };

  class database
  {
  public:
    database(const service_provider & sp);
    ~database();

    void open(const filename & fn);
    void close();

    void execute(const std::string & sql);
    sql_statement parse(const std::string & sql);

  private:
    _database * const impl_;
  };
  
  template <typename... argument_types>
  class prepared_statement
  {
  public:
    
    void execute(database & db, const char * sql, argument_types... arguments)
    {
      std::lock_guard<std::mutex> lock(this->mutex_);
      
      if (!this->statement_) {
        this->statement_.reset(new sql_statement(db.parse(sql)));
      }

      this->statement_->set_parameters(arguments...);
      this->statement_->execute();
    }
    
  private:
    std::mutex mutex_;
    std::unique_ptr<sql_statement> statement_;
  };
  
  template <typename... argument_types>
  class prepared_query
  {
  public:
    
    void query(
      database & db,
      const char * sql,
      const std::function<bool (sql_statement & st)> & callback,
      argument_types... arguments)
    {
      std::lock_guard<std::mutex> lock(this->mutex_);
      
      if (!this->statement_) {
        this->statement_.reset(new sql_statement(db.parse(sql)));
      }

      this->statement_->set_parameters(arguments...);
      while(this->statement_->execute()){
        if(!callback(*this->statement_)){
          break;
        }
      }
    }
    
  private:
    std::mutex mutex_;
    std::unique_ptr<sql_statement> statement_;

  };
}

#endif//__VDS_DATABASE_DATABASE_H_