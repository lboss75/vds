/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "logger.h"
#include "file.h"
#include "persistence.h"
#include "string_format.h"

void vds::logger::operator()(
  const service_provider & sp,
  log_level level,
  const std::string & message) const
{
  log_record record{ level, sp.name(), message };

  this->log_writer_.write(sp, record);
}
/////////////////////////////////////////////////////////
vds::log_writer::log_writer(log_level level)
: log_level_(level)
{
}

/////////////////////////////////////////////////////////
vds::console_logger::console_logger(log_level level)
: log_writer(level), logger(*this, level)
{
}

void vds::console_logger::register_services(service_registrator & registrator)
{
  registrator.add_service<logger>(this);
}

void vds::console_logger::start(const service_provider &)
{
}

void vds::console_logger::stop(const service_provider &)
{
}

void vds::console_logger::write(const service_provider & sp, const log_record & record)
{
  switch (record.level)
  {
  case ll_trace:
    std::cout << '[' << record.source << ']' << "TRACE: " << record.message << '\n';
    break;

  case ll_debug:
    std::cout << '[' << record.source << ']' << "DEBUG: " << record.message << '\n';
    break;

  case ll_info:
    std::cout << '[' << record.source << ']' << "INFO: " << record.message << '\n';
    break;

  case ll_warning:
    std::cout << '[' << record.source << ']' << "WARNIG: " << record.message << '\n';
    break;

  case ll_error:
    std::cout << '[' << record.source << ']' << "ERROR: " << record.message << '\n';
    break;
  }
}

std::string vds::exception_what(std::exception_ptr ex)
{
  std::string result;
  try{
    std::rethrow_exception(ex);
  }
  catch(std::exception * exp) {
    result = exp->what();
    delete exp;
  }
  catch(const std::exception & exp) {
    result = exp.what();
  }
  catch(...) {
    result = "Unknown error";
  }
  
  return result;

}
/////////////////////////////////////////////////////////

vds::file_logger::file_logger(log_level level)
: log_writer(level), logger(*this, level)
{
}

void vds::file_logger::register_services(service_registrator & registrator)
{
  registrator.add_service<logger>(this);
}

void vds::file_logger::start(const service_provider & sp)
{
  foldername folder(persistence::current_user(sp), ".vds");
  folder.create();
  this->f_.reset(new file(filename(folder, "vds.log"), file::file_mode::append));
}

void vds::file_logger::stop(const service_provider &)
{
  this->f_->close();
  this->f_.reset();
}

void vds::file_logger::write(
  const service_provider & sp,
  const log_record & record)
{
  std::string level_str;
  switch (record.level) {
  case ll_trace:
    level_str = "TRACE";
    break;

  case ll_debug:
    level_str = "DEBUG";
    break;

  case ll_info:
    level_str = "INFO";
    break;

  case ll_warning:
    level_str = "WARNIG";
    break;

  case ll_error:
    level_str = "ERROR";
    break;
  }

  std::stringstream s;
  auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  auto tm = std::localtime(&t);

  auto str = string_format(
    "%04d/%02d/%0d %02d:%02d.%02d %-6s %s: %s\n",
    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
    level_str.c_str(), record.source.c_str(), record.message.c_str());

  std::lock_guard<std::mutex> lock(this->file_mutex_);
  if (this->f_) {
    this->f_->write(str.c_str(), str.length());
    this->f_->flush();
  }
}
