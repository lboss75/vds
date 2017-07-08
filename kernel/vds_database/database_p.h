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
        auto error = sqlite3_errmsg(db);
        throw std::runtime_error(error);
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
      
      sqlite3_bind_blob(this->stmt_, index, value.data(), (int)value.size(), SQLITE_TRANSIENT);
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

      auto size = sqlite3_column_bytes(this->stmt_, index);
      if (0 >= size) {
        value.reset(nullptr, 0);
        return false;
      }
      else {
        auto v = sqlite3_column_blob(this->stmt_, index);
        value.reset(v, size);
        return true;
      }
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

  class _database_transaction : public std::enable_shared_from_this<_database_transaction>
  {
  public:
    _database_transaction(const filename & database_file)
    : db_(nullptr)
    {
      auto error = sqlite3_open(database_file.local_name().c_str(), &this->db_);

      if (SQLITE_OK != error) {
        throw std::runtime_error(sqlite3_errmsg(this->db_));
      }
    }

    ~_database_transaction()
    {
      this->close();
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

  class _database
  {
  public:
    static const size_t MAX_CONNECTION_POOL = 10;

    _database()
      : is_closed_(true)
    {
    }

    ~_database()
    {
      this->close();
    }

    void open(const filename & fn)
    {
      if (!this->is_closed_) {
        throw std::runtime_error("Attempted to open a database that is already open");
      }

      this->database_file_ = fn;
      this->is_closed_ = false;
    }

    void close()
    {
      if (!this->is_closed_) {
        std::lock_guard<std::mutex> lock(this->pool_mutex_);

        while (!this->connection_pool_.empty()) {
          this->connection_pool_.pop_front();
        }

        this->is_closed_ = true;
      }
    }

    database_transaction begin_transaction()
    {
      if (this->is_closed_) {
        throw std::runtime_error("Database is not opened");
      }

      auto result = this->create_transaction();
      result.execute("BEGIN TRANSACTION");
      return result;
    }

    void commit(database_transaction & t)
    {
      try {
        t.execute("COMMIT TRANSACTION");
      }
      catch (...) {
        this->free_transaction(t);
        throw;
      }

      this->free_transaction(t);
    }

    void rollback(database_transaction & t)
    {
      try {
        t.execute("ROLLBACK TRANSACTION");
      }
      catch (...) {
        this->free_transaction(t);
        throw;
      }

      this->free_transaction(t);
    }

  private:
    filename database_file_;
    bool is_closed_;

    std::mutex pool_mutex_;
    std::list<std::shared_ptr<_database_transaction>> connection_pool_;

    database_transaction create_transaction()
    {
      this->pool_mutex_.lock();
      if (!this->connection_pool_.empty()) {
        database_transaction result(this->connection_pool_.front());
        this->connection_pool_.pop_front();
        this->pool_mutex_.unlock();

        return result;
      }

      this->pool_mutex_.unlock();
      return database_transaction(std::make_shared<_database_transaction>(this->database_file_));
    }

    void free_transaction(database_transaction & t)
    {
      std::lock_guard<std::mutex> lock(this->pool_mutex_);
      if (!this->is_closed_ && MAX_CONNECTION_POOL > this->connection_pool_.size()) {
        this->connection_pool_.push_back(t.impl_);
      }
    }
  };
}



#endif//__VDS_DATABASE_DATABASE_P_H_
