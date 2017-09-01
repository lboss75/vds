#ifndef __VDS_CORE_LOGGER_H_
#define __VDS_CORE_LOGGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <sstream>
#include <mutex>
#include <memory>
#include <unordered_set>

#include "service_provider.h"
#include "string_format.h"

namespace vds {
    enum log_level {
        ll_trace,
        ll_debug,
        ll_info,
        ll_warning,
        ll_error
    };

    struct log_record
    {
      log_level level;
      std::string module;
      std::string source;
      std::string message;
    };

    class log_writer
    {
    public:
      log_writer(log_level level);

      virtual void write(const service_provider & sp, const log_record & record) = 0;

      log_level level() const {
        return this->log_level_;
      }

    private:
      log_level log_level_;
    };

    class logger {
    public:
      logger(log_writer & log_writer, log_level min_log_level, const std::unordered_set<std::string> & modules)
        : log_writer_(log_writer), min_log_level_(min_log_level), modules_(modules),
          all_modules_(modules.end() != modules.find("*"))
      {
      }

      template <typename... arg_types>
      void operator () (const std::string & module, const service_provider & sp, log_level level, const std::string & format, arg_types... args) const
      {
        if (this->check(module, sp, level)) {
          (*this)(module, sp, level, string_format(format, args...));
        }
      }

      template <typename... arg_types>
      void trace(const std::string & module, const service_provider & sp, const std::string & format, arg_types... args) const
      {
        if(this->check(module, sp, ll_trace)) {
          (*this)(module, sp, ll_trace, string_format(format, args...));
        }
      }

      template <typename... arg_types>
      void debug(const std::string & module, const service_provider & sp, const std::string & format, arg_types... args) const
      {
        if (this->check(module, sp, ll_debug)) {
          (*this)(module, sp, ll_debug, string_format(format, args...));
        }
      }

      template <typename... arg_types>
      void info(const std::string & module, const service_provider & sp, const std::string & format, arg_types... args) const
      {
        if (this->check(module, sp, ll_info)) {
          (*this)(module, sp, ll_info, string_format(format, args...));
        }
      }

      template <typename... arg_types>
      void warning(const std::string & module, const service_provider & sp, const std::string & format, arg_types... args) const
      {
        if (this->check(module, sp, ll_warning)) {
          (*this)(module, sp, ll_warning, string_format(format, args...));
        }
      }

      template <typename... arg_types>
      void error(const std::string & module, const service_provider & sp, const std::string & format, arg_types... args) const
      {
        if (this->check(module, sp, ll_error)) {
          (*this)(module, sp, ll_error, string_format(format, args...));
        }
      }

      log_level min_log_level() const {
        return this->min_log_level_;
      }
      
      static logger * get(service_provider & sp)
      {
        return sp.get<logger>();
      }

      static inline std::string escape_string(const std::string & str) {
        std::string result = str;
        replace_string(result, "\\", "\\\\");
        replace_string(result, "\n", "\\n");
        return result;
      }

      void set_log_level(log_level new_level)
      {
        this->min_log_level_ = new_level;
      }

    private:
      log_writer & log_writer_;
      log_level min_log_level_;
      bool all_modules_;
      std::unordered_set<std::string> modules_;
      
//       mutable std::mutex processed_modules_mutex_;
//       mutable std::unordered_set<std::string> processed_modules_;

      bool check(const std::string & module, const service_provider & sp, log_level level) const
      {
        if (this->min_log_level() > level) {
          return false;
        }
        
        if(this->modules_.end() != this->modules_.find(module)) {
          return true;
        }
        
        if(this->modules_.end() != this->modules_.find("-" + module)) {
          return false;
        }
        
//         this->processed_modules_mutex_.lock();
//         if(this->processed_modules_.end() == this->processed_modules_.find(module)){
//           this->processed_modules_.emplace(module);
//           this->processed_modules_mutex_.unlock();
//           
//           (*this)(module, sp, ll_warning, "Log messages of this module will be filtered");
//         }
//         else {
//           this->processed_modules_mutex_.unlock();
//         }
        
        return this->all_modules_;
      }
      
      void operator () (
        const std::string & module,
        const service_provider & sp,
        log_level level,
        const std::string & message) const;
    };

    class console_logger : public iservice_factory, public log_writer, public logger
    {
    public:
      console_logger(log_level level, const std::unordered_set<std::string> & modules);

      //iservice_factory
      void register_services(service_registrator &) override;
      void start(const service_provider &) override;
      void stop(const service_provider &) override;

      //log_writer
      void write(const service_provider & sp, const log_record & record) override;
    };

    class file;
    class file_logger : public iservice_factory, public log_writer, public logger
    {
    public:
      file_logger(log_level level, const std::unordered_set<std::string> & modules);

      //iservice_factory
      void register_services(service_registrator &) override;
      void start(const service_provider & sp) override;
      void stop(const service_provider & sp) override;

      //log_writer
      void write(const service_provider & sp, const log_record & record) override;

    private:
      std::mutex file_mutex_;
      std::unique_ptr<file> f_;
    };

}

#endif//__VDS_CORE_LOGGER_H_
