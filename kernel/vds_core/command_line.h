#ifndef __VDS_CORE_COMMAND_LINE_H_
#define __VDS_CORE_COMMAND_LINE_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <map>
#include <list>
#include "filename.h"

namespace vds {
  class command_line_item_error : public std::runtime_error
  {
  public:
    command_line_item_error(
      const std::string & message,
      const std::string & item_name);
  };
  
  class command_line_item
  {
  public:
    command_line_item(
      const std::string & name,
      const std::string & description);
    virtual ~command_line_item();
    
    virtual int try_parse(int argc, const char** argv) = 0;
    virtual void print_usage(std::map<std::string, const command_line_item *> & items) const = 0;
    virtual void print_help() const = 0;
    virtual void clear() = 0;
    
    const std::string & name() const {
      return this->name_;
    }
    
    const std::string & description() const {
      return this->description_;
    }
    
  private:
    std::string name_;
    std::string description_;
  };
  
  class command_line_value : public command_line_item
  {
  public:
    command_line_value(
      const std::string & sort_switch,
      const std::string & long_switch,
      const std::string & name,
      const std::string & description
    );
    
    const std::string & value() const;
    int try_parse(int argc, const char** argv) override;
    void print_usage(std::map<std::string, const command_line_item *> & items) const override;
    void print_help() const override;
    void clear() override;
    
  private:
    std::string sort_switch_;
    std::string long_switch_;
    std::string value_;
  };
  
  class command_line_values : public command_line_item
  {
  public:
    command_line_values(
      const std::string & sort_switch,
      const std::string & long_switch,
      const std::string & name,
      const std::string & description
    );
    
    const std::list<std::string> & values() const;
    int try_parse(int argc, const char** argv) override;
    void print_usage(std::map<std::string, const command_line_item *> & items) const override;
    void print_help() const override;
    void clear() override;
    
  private:
    std::string sort_switch_;
    std::string long_switch_;
    std::list<std::string> values_;
  };
  
  class command_line_switch : public command_line_item
  {
  public:
    command_line_switch(
      const std::string & sort_switch,
      const std::string & long_switch,
      const std::string & name,
      const std::string & description
    );
    
    bool value() const;
    
    int try_parse(int argc, const char** argv) override;
    void print_usage(std::map<std::string, const command_line_item *> & items) const override;
    void print_help() const override;
    void clear() override;
    
  private:
    std::string sort_switch_;
    std::string long_switch_;
    bool value_;
  };
  
  class command_line_set
  {
  public:
    command_line_set(
      const std::string & name,
      const std::string & description,
      const std::string & command = std::string(),
      const std::string & categoty = std::string()
    );
    
    void optional(command_line_item & );
    void required(command_line_item & );
    
    void print_usage(std::map<std::string, const command_line_item *> & items) const;
    int try_parse(
      int argc,
      const char** argv,
      std::string & last_error);
    
  private:
    std::string name_;
    std::string description_;
    std::string command_;
    std::string categoty_;
    
    std::list<command_line_item *> optional_;
    std::list<command_line_item *> required_;
  };
  
  class command_line
  {
  public:
    command_line(
      const std::string & name,
      const std::string & description,
      const std::string & version);
    
    void add_command_set(command_line_set &);
    
    const command_line_set * parse(int argc, const char** argv) const;
    
    void show_help(const std::string & app_name);

    void register_common_parameter(command_line_item & item)
    {
      for (auto cmd : this->command_sets_) {
        cmd->optional(item);
      }
    }

  private:
    std::string name_;
    std::string description_;
    std::string version_;
    std::list<command_line_set *> command_sets_;
  };
  
}

#endif // __VDS_CORE_COMMAND_LINE_H_
