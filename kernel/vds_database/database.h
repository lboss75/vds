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

    void set_parameter(int index, int value);
    void set_parameter(int index, const std::string & value);
    void set_parameter(int index, const guid & value);
    void set_parameter(int index, const data_buffer & value);

    bool execute();

    bool get_value(int index, int & value);
    bool get_value(int index, std::string & value);
    bool get_value(int index, guid & value);
    bool get_value(int index, data_buffer & value);

  private:
    _sql_statement * impl_;
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

}

#endif//__VDS_DATABASE_DATABASE_H_