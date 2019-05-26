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
    _sql_statement()
      : db_(nullptr), stmt_(nullptr), state_(bof_state) {
    }

    ~_sql_statement()
    {
      if (nullptr != this->stmt_) {
        sqlite3_finalize(this->stmt_);
      }
    }

    expected<void> create(sqlite3 * db, const char * sql)
    {
      this->db_ = db;
      vds_assert(this->stmt_ == nullptr);
      vds_assert(this->state_ == bof_state);

      auto result = sqlite3_prepare_v2(db, sql, -1, &this->stmt_, nullptr);
      switch (result) {
      case SQLITE_OK:
        return expected<void>();

      default:
        auto error = sqlite3_errmsg(db);
        return vds::make_unexpected<std::runtime_error>(error);
      }
    }

    void set_parameter(int index, int value)
    {
      this->reset();

      sqlite3_bind_int(this->stmt_, index, value);
    }

    void set_parameter(int index, int64_t value)
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

    expected<bool> execute()
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
        return vds::make_unexpected<std::runtime_error>(error);
      }
    }

    bool get_value(int index, int & value)
    {
      vds_assert(read_state == this->state_);
      vds_assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      value = sqlite3_column_int(this->stmt_, index);
      return true;
    }

    bool get_value(int index, int64_t & value)
    {
      vds_assert(read_state == this->state_);
      vds_assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      value = sqlite3_column_int64(this->stmt_, index);
      return true;
    }

    bool get_value(int index, std::string & value)
    {
      vds_assert(read_state == this->state_);
      vds_assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      auto v = (const char *)sqlite3_column_text(this->stmt_, index);
      if(nullptr == v){
        return false;
      }
      
      value = v;
      return true;
    }

    bool get_value(int index, const_data_buffer & value)
    {
      vds_assert(read_state == this->state_);
      vds_assert(0 <= index && index < sqlite3_column_count(this->stmt_));

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
      vds_assert(read_state == this->state_);
      vds_assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      value = sqlite3_column_double(this->stmt_, index);
      return true;
    }

    bool get_value(int index, std::chrono::system_clock::time_point & value)
    {
      vds_assert(read_state == this->state_);
      vds_assert(0 <= index && index < sqlite3_column_count(this->stmt_));

      value = std::chrono::system_clock::from_time_t(
          sqlite3_column_int64(this->stmt_, index));
      return true;
    }

    bool is_null(int index) const {
      vds_assert(0 <= index && index < sqlite3_column_count(this->stmt_));
      return (SQLITE_NULL == sqlite3_column_type(this->stmt_, index));
    }

  private:
    sqlite3 * db_;
    sqlite3_stmt * stmt_;
    
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
    : sp_(sp),
      db_(nullptr),
      execute_queue_(std::make_shared<thread_apartment>(sp))
    {
    }

    ~_database()
    {
      (void)this->close();
    }

    expected<void> open(const filename & database_file) {
      auto error = sqlite3_open(database_file.local_name().c_str(), &this->db_);

      if (SQLITE_OK != error) {
        return vds::make_unexpected<std::runtime_error>(sqlite3_errmsg(this->db_));
      }

      error = sqlite3_busy_timeout(this->db_, 300000);
      if (SQLITE_OK != error) {
        return vds::make_unexpected<std::runtime_error>(sqlite3_errmsg(this->db_));
      }

      sqlite3_trace_v2(this->db_, SQLITE_TRACE_STMT, tracer, (void *)this->sp_);

      return expected<void>();
    }

    expected<void> close() {
      if (nullptr != this->db_) {
        auto error = sqlite3_close(this->db_);

        if (SQLITE_OK != error) {
          auto error_msg = sqlite3_errmsg(this->db_);
          return vds::make_unexpected<std::runtime_error>(error_msg);
        }

        this->db_ = nullptr;
      }

      return expected<void>();
    }

    expected<void> execute(const char * sql)
    {
      char * zErrMsg = nullptr;
      auto result = sqlite3_exec(this->db_, sql, nullptr, 0, &zErrMsg);
      switch (result) {
      case SQLITE_OK:
        return expected<void>();

      default:
        if(nullptr != zErrMsg){
          std::string error_message(zErrMsg);
          sqlite3_free(zErrMsg);

          return vds::make_unexpected<std::runtime_error>(error_message);
        }
        else {
          return vds::make_unexpected<std::runtime_error>("Sqlite3 error " + std::to_string(result));
        }
      }
    }

    expected<sql_statement> parse(const char * sql) const
    {
      auto result = std::make_unique<_sql_statement>();
      CHECK_EXPECTED(result->create(this->db_, sql));
      return sql_statement(result.release());
    }

    async_task<expected<void>> async_read_transaction(
      const std::function<expected<void>(database_read_transaction & tr)> & callback) {

      auto r = std::make_shared<vds::async_result<vds::expected<void>>>();
      this->execute_queue_->schedule([pthis = this->shared_from_this(), r, callback]() -> expected<void> {
        thread_protect protect;

        database_read_transaction tr(pthis);
        auto callback_result = callback(tr);

        if(callback_result.has_error()) {
          pthis->sp_->get<logger>()->trace("DB", "%s at read transaction", callback_result.error()->what());
          r->set_value(unexpected(std::move(callback_result.error())));
        }
        else {
          r->set_value(expected<void>());
        }
        return expected<void>();
      });

      return r->get_future();
    }

    async_task<expected<void>> async_transaction(
      const std::function<expected<bool>(database_transaction & tr)> & callback) {
      auto r = std::make_shared<vds::async_result<vds::expected<void>>>();

      this->execute_queue_->schedule([pthis = this->shared_from_this(), r, callback]() -> expected<void> {
        thread_protect protect;
        CHECK_EXPECTED(pthis->execute("BEGIN TRANSACTION"));

        database_transaction tr(pthis);
        auto callback_result = callback(tr);
        if(callback_result.has_error()) {
          CHECK_EXPECTED(pthis->execute("ROLLBACK TRANSACTION"));
          pthis->sp_->get<logger>()->trace("DB", "%s at transaction", callback_result.error()->what());
          r->set_value(unexpected(std::move(callback_result.error())));
        }
        else if(callback_result.value()) {
          CHECK_EXPECTED(pthis->execute("COMMIT TRANSACTION"));
          r->set_value(expected<void>());
        }
        else {
          CHECK_EXPECTED(pthis->execute("ROLLBACK TRANSACTION"));
          r->set_value(expected<void>());
        }

        return expected<void>();
      });

      return r->get_future();
    }

    async_task<expected<void>> prepare_to_stop(){
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

    expected<int> last_insert_rowid() const {
      GET_EXPECTED(st, this->parse("select last_insert_rowid()"));
      if(!st.execute()) {
        return vds::make_unexpected<vds_exceptions::invalid_operation>();
      }

      int result;
      if(!st.get_value(0, result)) {
        return vds::make_unexpected<vds_exceptions::invalid_operation>();
      }

      return result;
    }

    size_t queue_length() const {
      return this->execute_queue_->size();
    }

  private:
    const service_provider * sp_;
    sqlite3 * db_;    
    std::shared_ptr<thread_apartment> execute_queue_;

    static int tracer(unsigned, void*sp, void *stmt, void*)
    {
      char *sql = sqlite3_expanded_sql((sqlite3_stmt*)stmt);
      static_cast<service_provider *>(sp)->get<logger>()->trace("DB", "%s", sql);
      sqlite3_free(sql);
      return 0;
    }
  };
}



#endif//__VDS_DATABASE_DATABASE_P_H_
