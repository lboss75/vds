/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "logger.h"
#include "file.h"
#include "persistence.h"
#include "string_format.h"
#include "mt_service.h"

void vds::logger::operator()(
  const std::string & module,
  log_level level,
  const std::string & message) const
{
  log_record record{ level, module, message };

  this->log_writer_.write(record);
}

/////////////////////////////////////////////////////////
vds::log_writer::log_writer(log_level level)
: log_level_(level)
{
}

/////////////////////////////////////////////////////////
vds::console_logger::console_logger(log_level level, const std::unordered_set<std::string> & modules)
: log_writer(level), logger(*this, level, modules)
{
}

vds::expected<void> vds::console_logger::register_services(service_registrator & registrator)
{
  registrator.add_service<logger>(this);
  return expected<void>();
}

vds::expected<void> vds::console_logger::start(const service_provider *)
{
  return expected<void>();
}

vds::expected<void> vds::console_logger::stop()
{
  return expected<void>();
}

void vds::console_logger::write( const log_record & record)
{
  switch (record.level)
  {
  case log_level::ll_trace:
    std::cout << "TRACE: " << record.message << '\n';
    break;

  case log_level::ll_debug:
    std::cout << "DEBUG: " << record.message << '\n';
    break;

  case log_level::ll_info:
    std::cout << "INFO: " << record.message << '\n';
    break;

  case log_level::ll_warning:
    std::cout << "WARNING: " << record.message << '\n';
    break;

  case log_level::ll_error:
    std::cout << "ERROR: " << record.message << '\n';
    break;
  }
}

void vds::console_logger::flush() {
  std::cout.flush();
}


/////////////////////////////////////////////////////////

vds::file_logger::file_logger(log_level level, const std::unordered_set<std::string> & modules)
  : log_writer(level), logger(*this, level, modules), is_stopping_(false)
{
}

vds::expected<void> vds::file_logger::register_services(service_registrator & registrator)
{
  registrator.add_service<logger>(this);
  return expected<void>();
}

vds::expected<void> vds::file_logger::start(const service_provider * sp)
{
  this->sp_ = sp;

  GET_EXPECTED(folder, persistence::current_user(sp));
  CHECK_EXPECTED(folder.create());

  file f;
  CHECK_EXPECTED(f.open(filename(folder, "vds.log"), file::file_mode::append));

  this->f_.reset(new file(std::move(f)));
  this->logger_thread_ = std::thread([this]() { this->logger_thread(); });

  return expected<void>();
}

vds::expected<void> vds::file_logger::stop()
{
  this->log_mutex_.lock();
  this->is_stopping_ = true;
  this->log_cond_.notify_all();
  this->log_mutex_.unlock();

  this->logger_thread_.join();

  (void)this->f_->close();
  this->f_.reset();

  return expected<void>();
}

void vds::file_logger::write(  const log_record & record)
{
  std::string level_str;
  switch (record.level) {
  case log_level::ll_trace:
    level_str = "TRACE";
    break;

  case log_level::ll_debug:
    level_str = "DEBUG";
    break;

  case log_level::ll_info:
    level_str = "INFO";
    break;

  case log_level::ll_warning:
    level_str = "WARNING";
    break;

  case log_level::ll_error:
    level_str = "ERROR";
    break;
  }

  std::stringstream s;
  auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  auto tm = std::localtime(&t);

  auto str = string_format(
    "%04d/%02d/%0d %02d:%02d.%02d %-6d %-6s [%-10s] %s\n",
    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
#ifndef _WIN32
    syscall(SYS_gettid),
#else
    GetCurrentThreadId(),
#endif
  level_str.c_str(), record.module.c_str(), record.message.c_str());

  std::lock_guard<std::mutex> lock(this->log_mutex_);
  this->log_ += str;
  this->log_cond_.notify_all();
}

void vds::file_logger::flush() {
  std::unique_lock<std::mutex> lock(this->log_mutex_);
  if (!this->log_.empty()) {
    auto log = this->log_;
    this->log_.clear();
    lock.unlock();

    (void)this->f_->write(log.c_str(), log.length());
    (void)this->f_->flush();
  }
}

void vds::file_logger::logger_thread() {
  for(;;) {
    std::unique_lock<std::mutex> lock(this->log_mutex_);
    for (;;) {
      if (this->is_stopping_) {
        return;
      }
      if (!this->log_.empty()) {
        break;
      }

      this->log_cond_.wait(lock);
    }
    auto log = this->log_;
    this->log_.clear();
    lock.unlock();

    (void)this->f_->write(log.c_str(), log.length());
    (void)this->f_->flush();
  }
}
