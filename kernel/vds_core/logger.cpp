/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "logger.h"

vds::logger::logger(const service_provider & sp, const std::string & name)
: sp_(sp), name_(name), log_writer_(sp.get_collection<log_writer>()), min_log_level_(ll_error)
{
  for (auto & p : this->log_writer_) {
    if (this->min_log_level_ > p.level()) {
      this->min_log_level_ = p.level();
    }
  }
}

void vds::logger::operator()(log_level level, const std::string & message) const
{
  log_record record{ level, this->name_, message };

    for (auto & p : this->log_writer_) {
        p.write(record);
    }
}
/////////////////////////////////////////////////////////
vds::log_writer::log_writer(log_level level, const std::function<void(const log_record & record)> & impl)
    : log_level_(level), impl_(impl)
{
}

void vds::log_writer::write(const log_record & record) const
{
  if (this->log_level_ <= record.level) {
    this->impl_(record);
  }
}
/////////////////////////////////////////////////////////
vds::console_logger::console_logger(log_level level)
  : level_(level)
{
}

void vds::console_logger::register_services(service_registrator & registrator)
{
    registrator.add_collection_factory<log_writer>([this](const service_provider &)->log_writer {
      return log_writer(this->level_, [](const log_record & record) {
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
      });
    });
}

void vds::console_logger::start(const service_provider &)
{
}

void vds::console_logger::stop(const service_provider &)
{
}

