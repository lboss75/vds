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
    {
    }
    
    void register_services(service_registrator & registrator)
    {
    }
    
    
  protected:
    void start()
    {
      vds::service_registrator registrator;

      static_cast<app_impl *>(this)->register_services(registrator);
      
      auto sp = registrator.build("application");
      try {
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
  };
  
  template <typename app_impl>
  class console_app : public app_base<app_impl>
  {
  public:
    console_app()
    :
      console_logger_(ll_trace),
      help_cmd_set_("Show help", "Show application help"),
      help_cmd_switch_("h", "help", "Help", "Show help")
    {
    }
    
    void register_services(service_registrator & registrator)
    {
      app_base<app_impl>::register_services(registrator);
      registrator.add(console_logger_);
    }
    
    void on_exception(std::exception_ptr ex)
    {
      std::cerr << exception_what(ex);
      exit(1);
    }
   
    void register_command_line(command_line & cmd_line)
    {
      cmd_line.add_command_set(this->help_cmd_set_);
      this->help_cmd_set_.required(this->help_cmd_switch_);
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
        
        this->current_command_set_ = cmd_line.parse(argc, argv);
        
        if(
          nullptr == this->current_command_set_
          || this->current_command_set_ == &this->help_cmd_set_){
          cmd_line.show_help(filename(argv[0]).name());
          return 0;
        }
        
        static_cast<app_impl *>(this)->start();
        return 0;
      }
      catch(...){
        static_cast<app_impl *>(this)->on_exception(std::current_exception());
	return 1;
      }
    }
    
  private:
    console_logger console_logger_;
    command_line_set help_cmd_set_;
    command_line_switch help_cmd_switch_;
  protected:
    const command_line_set * current_command_set_;
  };
}

#endif // __VDS_CORE_APP_H_
