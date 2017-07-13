#ifndef __VDS_DATABASE_DATABASE_P_H_
#define __VDS_DATABASE_DATABASE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>
#include <thread>

#include "sqllite3/sqlite3.h"
#include "debug.h"

namespace vds {
  class database;

  DECLARE_DEBUG_TASK(_database_transaction_task)
  DECLARE_DEBUG_TASK(_database_transaction_debug)

  void dump_commands();

  class _sql_statement
  {
  public:
    _sql_statement(sqlite3 * db, const char * sql)
      : db_(db), stmt_(nullptr), state_(bof_state)
    {
      for (auto try_count = 0;; ++try_count) {
        auto result = sqlite3_prepare_v2(db, sql, -1, &this->stmt_, nullptr);
        switch (result) {
        case SQLITE_OK:
          return;

        case SQLITE_BUSY:
          if (30 > try_count) {
            dump_commands();

            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
          }
          //break;

        default:
          auto error = sqlite3_errmsg(db);
          throw std::runtime_error(error);
        }
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
      for (auto try_count = 0; ; ++try_count) {
        auto result = sqlite3_step(this->stmt_);
        switch (result) {
        case SQLITE_ROW:
          this->state_ = read_state;
          return true;

        case SQLITE_DONE:
          this->state_ = eof_state;
          return false;

        case SQLITE_BUSY:
          if (30 > try_count) {
            dump_commands();

            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
          }
          //break;

        default:
          auto error = sqlite3_errmsg(this->db_);
          throw std::runtime_error(error);
        }
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
    _database_transaction(const service_provider & sp, const filename & database_file)
    : sp_(sp), db_(nullptr)
    {
      auto error = sqlite3_open(database_file.local_name().c_str(), &this->db_);

      if (SQLITE_OK != error) {
        throw std::runtime_error(sqlite3_errmsg(this->db_));
      }

      START_DEBUG_TASK(_database_transaction_debug, sp.full_name());
    }

    ~_database_transaction()
    {
      FINISH_DEBUG_TASK(_database_transaction_debug, this->sp_.full_name());
      this->close();
    }

    void close()
    {
      if (nullptr != this->db_) {
        sqlite3_close(this->db_);
        this->db_ = nullptr;
      }
    }

    void execute(const char * sql)
    {
      for (auto try_count = 0; ; ++try_count) {
        char * zErrMsg = nullptr;
        auto result = sqlite3_exec(this->db_, sql, nullptr, 0, &zErrMsg);
        switch (result) {
        case SQLITE_OK:
          return;

        case SQLITE_BUSY:
          if (30 > try_count) {
            sqlite3_free(zErrMsg);

            dump_commands();

            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
          }
          //break;

        default:
          std::string error_message(zErrMsg);
          sqlite3_free(zErrMsg);

          throw std::runtime_error(error_message);
        }
      }
    }

    sql_statement parse(const char * sql)
    {
      return sql_statement(new _sql_statement(this->db_, sql));
    }

  private:
    service_provider sp_;
    sqlite3 * db_;    
  };

  DECLARE_DEBUG_TASK(_database_commit_task)
  class _database
  {
  public:
    static const size_t MAX_CONNECTION_POOL = 0;

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

    database_transaction begin_transaction(const service_provider & sp)
    {
      if (this->is_closed_) {
        throw std::runtime_error("Database is not opened");
      }

      auto result = this->create_transaction(sp);
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

    database_transaction create_transaction(const service_provider & sp)
    {
      this->pool_mutex_.lock();
      if (!this->connection_pool_.empty()) {
        database_transaction result(this->connection_pool_.front());
        this->connection_pool_.pop_front();
        this->pool_mutex_.unlock();

        return result;
      }

      this->pool_mutex_.unlock();
      return database_transaction(std::make_shared<_database_transaction>(sp, this->database_file_));
    }

    void free_transaction(database_transaction & t)
    {
      std::lock_guard<std::mutex> lock(this->pool_mutex_);
      if (!this->is_closed_ && MAX_CONNECTION_POOL > this->connection_pool_.size()) {
        this->connection_pool_.push_back(t.impl_);
      }
    }
  };

  inline void dump_commands()
  {
    std::cout << "Waiting for " << _database_transaction_task << "\n";
    std::cout << "Transactions: " << _database_transaction_debug << "\n";
  }
}



#endif//__VDS_DATABASE_DATABASE_P_H_
