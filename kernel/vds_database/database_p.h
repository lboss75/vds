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


  class _database
  {
  public:
    _database()
    : sp_(service_provider::empty()), db_(nullptr)
    {
    }

    ~_database()
    {
      this->close();
    }
    
    void open(const service_provider & sp, const filename & database_file)
    {
      this->sp_ = sp;
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
        std::string error_message(zErrMsg);
        sqlite3_free(zErrMsg);

        throw std::runtime_error(error_message);
      }
    }

    sql_statement parse(const char * sql)
    {
      return sql_statement(new _sql_statement(this->db_, sql));
    }

    void async_transaction(
      const service_provider & sp,
      const std::function<bool(database_transaction & tr)> & callback)
    {
      mt_service::async(sp, [this, sp, callback](){
        this->callbacks_mutex_.lock();
        this->callbacks_.push_back(callback);
        if(1 < callbacks_.size()){
          this->callbacks_mutex_.unlock();
          return;
        }
        
        for(;;){
          auto & head = *this->callbacks_.begin();
          this->callbacks_mutex_.unlock();
          
          this->transaction_mutex_.lock();
          try{
            
            this->execute("BEGIN TRANSACTION");
            
            database_transaction tr(this);
            if(head(tr)){
              this->execute("COMMIT TRANSACTION");
            }
            else {
              this->execute("ROLLBACK TRANSACTION");
            }
          }
          catch(...){
            this->execute("ROLLBACK TRANSACTION");
          }
          this->transaction_mutex_.unlock();
          
          this->callbacks_mutex_.lock();
          this->callbacks_.pop_front();
          
          if(0 == this->callbacks_.size()){
            this->callbacks_mutex_.unlock();
            return;
          }
        }
      });
    }
    
    void sync_transaction(
      const service_provider & sp,
      const std::function<bool(database_transaction & tr)> & callback)
    {
      std::unique_lock<std::mutex> lock(this->transaction_mutex_);
        
      try{
        
        this->execute("BEGIN TRANSACTION");
        
        database_transaction tr(this);
        if(callback(tr)){
          this->execute("COMMIT TRANSACTION");
        }
        else {
          this->execute("ROLLBACK TRANSACTION");
        }
      }
      catch(...){
        this->execute("ROLLBACK TRANSACTION");
      }
    }

  private:
    service_provider sp_;
    filename database_file_;
    sqlite3 * db_;    
    std::mutex transaction_mutex_;
    std::mutex callbacks_mutex_;
    std::list<std::function<bool(database_transaction & tr)>> callbacks_;

  };

  inline void dump_commands()
  {
    std::cout << "Waiting for " << _database_transaction_task << "\n";
    std::cout << "Transactions: " << _database_transaction_debug << "\n";
  }
}



#endif//__VDS_DATABASE_DATABASE_P_H_
