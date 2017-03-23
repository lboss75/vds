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
      : db_(db), stmt_(nullptr)
    {
      if (SQLITE_OK != sqlite3_prepare_v2(db, sql.c_str(), -1, &this->stmt_, nullptr)) {
        throw new std::runtime_error(sqlite3_errmsg(db));
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
      sqlite3_bind_int(this->stmt_, index, value);
      this->eof_ = false;
    }

    void set_parameter(int index, const std::string & value)
    {
      sqlite3_bind_text(this->stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
      this->eof_ = false;
    }

    bool execute()
    {
      if (this->eof_) {
        return false;
      }

      switch (sqlite3_step(this->stmt_)) {
      case SQLITE_ROW:
        return true;

      case SQLITE_DONE:
        this->eof_ = true;
        return true;

      default:
        throw new std::runtime_error(sqlite3_errmsg(this->db_));
      }
    }

    bool get_value(int index, int & value)
    {
      value = sqlite3_column_int(this->stmt_, index);
      return true;
    }

    bool get_value(int index, std::string & value)
    {
      value = (const char *)sqlite3_column_text(this->stmt_, index);
      return true;
    }


  private:
    sqlite3 * db_;
    sqlite3_stmt * stmt_;
    bool eof_;
  };


  class _database
  {
  public:
    _database(const service_provider & sp, database * owner)
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
        throw new std::runtime_error(sqlite3_errmsg(this->db_));
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

        throw new std::runtime_error(error_message);
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
