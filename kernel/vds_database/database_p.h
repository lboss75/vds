#ifndef __VDS_DATABASE_DATABASE_P_H_
#define __VDS_DATABASE_DATABASE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "sqllite3/sqlite3.h"

namespace vds {
  class database;

  class _sql_statement
  {
  public:
    _sql_statement(sqlite3 * db, const std::string & sql)
      : db_(db), stmt_(nullptr), state_(bof_state)
    {
      if (SQLITE_OK != sqlite3_prepare_v2(db, sql.c_str(), -1, &this->stmt_, nullptr)) {
        throw std::runtime_error(sqlite3_errmsg(db));
      }
    }

    ~_sql_statement()
    {
      if (nullptr != this->stmt_) {
        sqlite3_finalize(this->stmt_);
      }
    }

    void set_parameter(int index, int value)
    {
      this->reset();

      sqlite3_bind_int(this->stmt_, index, value);
    }

    void set_parameter(int index, uint64_t value)
    {
      this->reset();
      
      sqlite3_bind_int64(this->stmt_, index, value);
    }

    void set_parameter(int index, const std::string & value)
    {
      this->reset();
      
      sqlite3_bind_text(this->stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
    }

    void set_parameter(int index, const guid & value)
    {
      this->reset();
      
      this->set_parameter(index, value.str());
    }

    void set_parameter(int index, const const_data_buffer & value)
    {
      this->reset();
      
      this->set_parameter(index, base64::from_bytes(value));
    }

    bool execute()
    {
      switch (sqlite3_step(this->stmt_)) {
      case SQLITE_ROW:
        this->state_ = read_state;
        return true;

      case SQLITE_DONE:
        this->state_ = eof_state;
        return false;

      default:
        throw std::runtime_error(sqlite3_errmsg(this->db_));
      }
    }

    bool get_value(int index, int & value)
    {
      assert(read_state == this->state_);

      value = sqlite3_column_int(this->stmt_, index);
      return true;
    }

    bool get_value(int index, uint64_t & value)
    {
      assert(read_state == this->state_);
      
      value = sqlite3_column_int64(this->stmt_, index);
      return true;
    }

    bool get_value(int index, std::string & value)
    {
      assert(read_state == this->state_);
      
      auto v = (const char *)sqlite3_column_text(this->stmt_, index);
      if(nullptr == v){
        return false;
      }
      
      value = v;
      return true;
    }

    bool get_value(int index, guid & value)
    {
      assert(read_state == this->state_);
      
      std::string v;
      if (!this->get_value(index, v)) {
        return false;
      }

      value = guid::parse(v);
      return true;
    }

    bool get_value(int index, const_data_buffer & value)
    {
      assert(read_state == this->state_);
      
      std::string v;
      if (!this->get_value(index, v)) {
        return false;
      }

      value = base64::to_bytes(v);
      return true;
    }


  private:
    //service_provider sp_;
    sqlite3 * db_;
    sqlite3_stmt * stmt_;
    //logger log_;
    //std::string query_;
    
    enum state_enum
    {
      bof_state,
      read_state,
      eof_state,
    };
    
    state_enum state_;
    
    void reset()
    {
      if(bof_state != this->state_){
        sqlite3_reset(this->stmt_);
        this->state_ = bof_state;
      }
    }
  };


  class _database
  {
  public:
    _database(database * /*owner*/)
      : db_(nullptr)
    {
    }

    ~_database()
    {
      this->close();
    }

    void open(const filename & fn)
    {
      auto error = sqlite3_open(fn.local_name().c_str(), &this->db_);

      if (SQLITE_OK != error) {
        throw std::runtime_error(sqlite3_errmsg(this->db_));
      }
    }

    void close()
    {
      if (nullptr != this->db_) {
        sqlite3_close(this->db_);
        this->db_ = nullptr;
      }
    }

    void execute(const std::string & sql)
    {
      char * zErrMsg = nullptr;
      auto error = sqlite3_exec(this->db_, sql.c_str(), nullptr, 0, &zErrMsg);
      if (SQLITE_OK != error) {
        std::string error_message(zErrMsg);
        sqlite3_free(zErrMsg);

        throw std::runtime_error(error_message);
      }
    }

    sql_statement parse(const std::string & sql)
    {
      return sql_statement(new _sql_statement(this->db_, sql));
    }

  private:
    sqlite3 * db_;
  };
}



#endif//__VDS_DATABASE_DATABASE_P_H_
