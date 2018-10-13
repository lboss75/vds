#ifndef __VDS_DATABASE_DATABASE_P_H_
#define __VDS_DATABASE_DATABASE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>
#include <thread>

#include "sqllite3/sqlite3.h"
#include "thread_apartment.h"
#include "vds_exceptions.h"

namespace vds {
  class database;

  class _sql_statement
  {
  public:
    _sql_statement(sqlite3 * db, const char * sql)
      : db_(db), stmt_(nullptr), state_(bof_state)
    {
      auto result = sqlite3_prepare_v2(db, sql, -1, &this->stmt_, nullptr);
      switch (result) {
      case SQLITE_OK:
        return;

      default:
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

    void set_parameter(int index, const std::chrono::system_clock::time_point & value)
    {
      this->reset();

      sqlite3_bind_int64(this->stmt_, index, std::chrono::system_clock::to_time_t(value));
    }


    void set_parameter(int index, const const_data_buffer & value)
    {
      this->reset();
      
      sqlite3_bind_blob(this->stmt_, index, value.data(), (int)value.size(), SQLITE_TRANSIENT);
    }

    bool execute()
    {
      auto result = sqlite3_step(this->stmt_);
      switch (result) {
      case SQLITE_ROW:
        this->state_ = read_state;
        return true;

      case SQLITE_DONE:
        this->state_ = eof_state;
        return false;

      default:
        auto error = sqlite3_errmsg(this->db_);
        throw std::runtime_error(error);
      }
    }

    bool get_value(int index, int & value)
    {
      assert(read_state == this->state_);
      assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      value = sqlite3_column_int(this->stmt_, index);
      return true;
    }

    bool get_value(int index, uint64_t & value)
    {
      assert(read_state == this->state_);
      assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      value = sqlite3_column_int64(this->stmt_, index);
      return true;
    }

    bool get_value(int index, std::string & value)
    {
      assert(read_state == this->state_);
      assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      auto v = (const char *)sqlite3_column_text(this->stmt_, index);
      if(nullptr == v){
        return false;
      }
      
      value = v;
      return true;
    }

    bool get_value(int index, const_data_buffer & value)
    {
      assert(read_state == this->state_);
      assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      const auto size = sqlite3_column_bytes(this->stmt_, index);
      if (0 >= size) {
        value.resize(0);
        return false;
      }
      else {
        auto v = sqlite3_column_blob(this->stmt_, index);
        value = const_data_buffer(v, size);
        return true;
      }
    }

    bool get_value(int index, double & value)
    {
      assert(read_state == this->state_);
      assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      value = sqlite3_column_double(this->stmt_, index);
      return true;
    }

    bool get_value(int index, std::chrono::system_clock::time_point & value)
    {
      assert(read_state == this->state_);
      assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      value = std::chrono::system_clock::from_time_t(
          sqlite3_column_int64(this->stmt_, index));
      return true;
    }

    bool is_null(int index) const {
      assert(0 <= index && index < sqlite3_column_count(this->stmt_));
      return (SQLITE_NULL == sqlite3_column_type(this->stmt_, index));
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


  class _database : public std::enable_shared_from_this<_database>
  {
  public:
    _database(const service_provider * sp)
    : db_(nullptr),
      execute_queue_(std::make_shared<thread_apartment>(sp))
    {
    }

    ~_database()
    {
      this->close();
    }
    
    void open(const filename & database_file)
    {
      auto error = sqlite3_open(database_file.local_name().c_str(), &this->db_);

      if (SQLITE_OK != error) {
        throw std::runtime_error(sqlite3_errmsg(this->db_));
      }

      error = sqlite3_busy_timeout(this->db_, 300000);
      if (SQLITE_OK != error) {
        throw std::runtime_error(sqlite3_errmsg(this->db_));
      }
    }

    void close()
    {
      if (nullptr != this->db_) {
        auto error = sqlite3_close(this->db_);

        if (SQLITE_OK != error) {
          auto error_msg = sqlite3_errmsg(this->db_);
          throw std::runtime_error(error_msg);
        }

        this->db_ = nullptr;
      }
    }

    void execute(const char * sql)
    {
      char * zErrMsg = nullptr;
      auto result = sqlite3_exec(this->db_, sql, nullptr, 0, &zErrMsg);
      switch (result) {
      case SQLITE_OK:
        return;

      default:
        if(nullptr != zErrMsg){
          std::string error_message(zErrMsg);
          sqlite3_free(zErrMsg);

          throw std::runtime_error(error_message);
        }
        else {
          throw std::runtime_error("Sqlite3 error " + std::to_string(result));
        }
      }
    }

    sql_statement parse(const char * sql) const
    {
      return sql_statement(new _sql_statement(this->db_, sql));
    }

    vds::async_task<void> async_read_transaction(
      
      const std::function<void(database_read_transaction & tr)> & callback) {

      auto r = std::make_shared<vds::async_result<void>>();
      this->execute_queue_->schedule([pthis = this->shared_from_this(), r, callback]() {
        database_read_transaction tr(pthis);
        try {
          callback(tr);
        }
        catch(...) {
          r->set_exception(std::current_exception());
          return;
        }

        r->set_value();
      });

      return r->get_future();
    }

    vds::async_task<void> async_transaction(
      
      const std::function<bool(database_transaction & tr)> & callback) {
      auto r = std::make_shared<vds::async_result<void>>();

      this->execute_queue_->schedule([pthis = this->shared_from_this(), r, callback]() {
        pthis->execute("BEGIN TRANSACTION");

        database_transaction tr(pthis);

        bool result;
        try {
          result = callback(tr);
        }
        catch (...) {
          pthis->execute("ROLLBACK TRANSACTION");
          r->set_exception(std::current_exception());
          return;
        }

        if (result) {
          pthis->execute("COMMIT TRANSACTION");
        }
        else {
          pthis->execute("ROLLBACK TRANSACTION");
        }

        r->set_value();
      });

      return r->get_future();
    }

    vds::async_task<void> prepare_to_stop(){
      return this->execute_queue_->prepare_to_stop();
    }


    /**
     * \brief This function returns the number of rows modified, inserted or deleted by 
     * the most recently completed INSERT, UPDATE or DELETE statement on the database connection
     * \return Count The Number Of Rows Modified
     */
    int rows_modified() const {
      return sqlite3_changes(this->db_);
    }

    int last_insert_rowid() const {
      auto st = this->parse("select last_insert_rowid()");
      if(!st.execute()) {
        throw vds_exceptions::invalid_operation();
      }

      int result;
      if(!st.get_value(0, result)) {
        throw vds_exceptions::invalid_operation();
      }

      return result;
    }

  private:
    sqlite3 * db_;    
    std::shared_ptr<thread_apartment> execute_queue_;
  };
}



#endif//__VDS_DATABASE_DATABASE_P_H_
