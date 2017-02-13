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
    for (auto & p : this->log_writer_) {
        p.write(level, message);
    }
}
/////////////////////////////////////////////////////////
vds::log_writer::log_writer(log_level level, const std::function<void(log_level level, const std::string & message)> & impl)
    : log_level_(level), impl_(impl)
{
}

void vds::log_writer::write(log_level level, const std::string & message) const
{
  if (this->log_level_ <= level) {
    this->impl_(level, message);
  }
}
/////////////////////////////////////////////////////////
vds::console_logger::console_logger(log_level level)
  : level_(level)
{
}

void vds::console_logger::register_services(service_registrator & registrator)
{
    registrator.add_collection_factory<log_writer>([this]()->log_writer {
      return log_writer(this->level_, [](log_level level, const std::string & message) {
        switch (level)
        {
        case ll_trace:
          std::cout << "TRACE: " << message << '\n';
          break;

        case ll_debug:
          std::cout << "DEBUG: " << message << '\n';
          break;

        case ll_info:
          std::cout << "INFO: " << message << '\n';
          break;

        case ll_error:
          std::cout << "ERROR: " << message << '\n';
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

