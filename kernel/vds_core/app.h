#ifndef __VDS_CORE_APP_H_
#define __VDS_CORE_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>
#include <exception>
#include <iostream>
 
#include "service_provider.h"
#include "logger.h"
#include "command_line.h"
#include "filename.h"

namespace vds{
  
  class app
  {
  public:
    std::string app_name() const {
      return "VDS application";
    }
    
    std::string app_description() const {
      return "Distributed file system";
    }
    
    std::string app_version() const {
      return "0.1";
    }
    
  protected:
    
  };
  
  template <typename app_impl>
  class app_base : public app
  {
  public:
    app_base()
    : logger_(ll_info),
      log_level_("ll", "log_level", "Log Level", "Set log level"),
      current_command_set_(nullptr),
      help_cmd_set_("Show help", "Show application help"),
      help_cmd_switch_("h", "help", "Help", "Show help")
    {
    }

    int run(int argc, const char ** argv)
    {
      setlocale(LC_ALL, "Russian");

      try {
        command_line cmd_line(
          static_cast<app_impl *>(this)->app_name(),
          static_cast<app_impl *>(this)->app_description(),
          static_cast<app_impl *>(this)->app_version()
        );

        static_cast<app_impl *>(this)->register_command_line(cmd_line);
        static_cast<app_impl *>(this)->register_common_parameters(cmd_line);

        this->current_command_set_ = cmd_line.parse(argc, argv);

        static_cast<app_impl *>(this)->process_common_parameters();

        if (
          nullptr == this->current_command_set_
          || this->current_command_set_ == &this->help_cmd_set_) {
          cmd_line.show_help(filename(argv[0]).name_without_extension());
          return 0;
        }

        static_cast<app_impl *>(this)->start();
        return 0;
      }
      catch (std::exception & ex) {
        static_cast<app_impl *>(this)->on_exception(std::make_shared<std::runtime_error>(ex.what()));
        return 1;
      }
      catch (...) {
        static_cast<app_impl *>(this)->on_exception(std::make_shared<std::runtime_error>("Unexpected error"));
        return 1;
      }
    }

  protected:
    file_logger logger_;
    command_line_value log_level_;
    const command_line_set * current_command_set_;

    void start()
    {
      vds::service_registrator registrator;

      static_cast<app_impl *>(this)->register_services(registrator);
      
      auto sp = registrator.build("application");
      try {
        static_cast<app_impl *>(this)->start_services(registrator, sp);
        static_cast<app_impl *>(this)->before_main(sp);
        static_cast<app_impl *>(this)->main(sp);
      }
      catch (...) {
        try {
          registrator.shutdown(sp);
        }
        catch (...) { }        
        throw;
      }     
      
      registrator.shutdown(sp);
    }
    void register_services(service_registrator & registrator)
    {
      registrator.add(this->logger_);
    }

    void start_services(service_registrator & registrator, service_provider & sp)
    {
      registrator.start(sp);
    }

    void before_main(service_provider & sp)
    {
      sp.set_property<unhandled_exception_handler>(
        service_provider::property_scope::any_scope,
        new vds::unhandled_exception_handler(
          [this](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        static_cast<app_impl *>(this)->on_exception(ex);
      }));
    }

    void on_exception(const std::shared_ptr<std::exception> & ex)
    {
      exit(1);
    }

    void register_command_line(command_line & cmd_line)
    {
      cmd_line.add_command_set(this->help_cmd_set_);
      this->help_cmd_set_.required(this->help_cmd_switch_);
    }

    void register_common_parameters(command_line & cmd_line)
    {
      cmd_line.register_common_parameter(this->log_level_);
    }

    void process_common_parameters()
    {
      if ("trace" == this->log_level_.value()) {
        this->logger_.set_log_level(ll_trace);
      }
      else if ("debug" == this->log_level_.value()) {
        this->logger_.set_log_level(ll_debug);
      }
      else if ("info" == this->log_level_.value()) {
        this->logger_.set_log_level(ll_info);
      }
      else if ("error" == this->log_level_.value()) {
        this->logger_.set_log_level(ll_error);
      }
    }

  private:
    command_line_set help_cmd_set_;
    command_line_switch help_cmd_switch_;
  };
  
  template <typename app_impl>
  class console_app : public app_base<app_impl>
  {
  public:
    console_app()
    {
    }
    
    void on_exception(const std::shared_ptr<std::exception> & ex)
    {
      std::cerr << ex->what();
      app_base<app_impl>::on_exception(ex);
    }
  };
}

#endif // __VDS_CORE_APP_H_
