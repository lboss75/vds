#ifndef __VDS_PARSER_JSON_OBJECT_H_
#define __VDS_PARSER_JSON_OBJECT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <list>
#include <string>
#include "const_data_buffer.h"

namespace vds {
  class json_writer;

  class json_value : public std::enable_shared_from_this<json_value>
  {
  public:
    json_value();
    json_value(int line, int column);
    virtual ~json_value();

    int line() const {
      return this->line_;
    }
    
    int column() const {
      return this->column_;
    }
    
    std::string str() const;
    
    virtual void str(json_writer & writer) const = 0;
    virtual std::shared_ptr<json_value> clone(bool is_deep_clone) const = 0;
    
  private:
    int line_;
    int column_;
  };
  
  class json_primitive : public json_value
  {
  public:
    json_primitive(const std::string & value);
    json_primitive(
      int line, int column,
      const std::string & value);
    
    const std::string & value() const {
      return this->value_;
    }
    
    void str(json_writer & writer) const override;
    std::shared_ptr<json_value> clone(bool is_deep_clone) const override;

  private:
    std::string value_;
  };
  
  class json_property : public json_value
  {
  public:
    json_property(int line, int column);
    json_property(const std::string & name, const std::shared_ptr<json_value> & val);

    const std::string & name() const {
      return this->name_;
    }

    const std::shared_ptr<json_value> & value() const {
      return this->value_;
    }

    void name(const std::string & value) {
      this->name_ = value;
    }

    void value(const std::shared_ptr<json_value> & val) {
      this->value_ = val;
    }
    
    void str(json_writer & writer) const override;
    std::shared_ptr<json_value> clone(bool is_deep_clone) const override;

  private:
    friend class json_object;
    
    std::string name_;
    std::shared_ptr<json_value> value_;
  };
  
  class json_object : public json_value
  {
  public:
    json_object();
    json_object(int line, int column);

    void visit(const std::function<void(const std::shared_ptr<json_property> &)> & visitor) const;
    
    void add_property(const std::shared_ptr<json_property> & prop);
    void add_property(const std::string & name, const std::shared_ptr<json_value> & value);
    void add_property(const std::string & name, uint64_t value);
    void add_property(const std::string & name, const std::chrono::system_clock::time_point & value);
    void add_property(const std::string & name, const std::chrono::steady_clock::time_point & value);
    void add_property(const std::string & name, const std::string & value);
    void add_property(const std::string & name, const const_data_buffer & value);
    void add_property(const std::string & name, const std::list<const_data_buffer> & value);
    void add_property(const std::string & name, const std::list<uint16_t> & value);

    template<typename item_type>
    void add_property(const std::string & name, const std::list<item_type> & value);

    std::shared_ptr<json_value> get_property(const std::string & name) const;
    bool get_property(const std::string & name, std::string & value, bool throw_error = true) const;
    bool get_property(const std::string & name, int & value, bool throw_error = true) const;
    //bool get_property(const std::string & name, size_t & value, bool throw_error = true) const;
    bool get_property(const std::string & name, const_data_buffer & value, bool throw_error = true) const;
    bool get_property(const std::string & name, uint8_t & value, bool throw_error = true) const;
    bool get_property(const std::string & name, uint16_t & value, bool throw_error = true) const;
    bool get_property(const std::string & name, uint32_t & value, bool throw_error = true) const;
    bool get_property(const std::string & name, uint64_t & value, bool throw_error = true) const;
    bool get_property(const std::string & name, std::list<const_data_buffer> & value, bool throw_error = true) const;

    template<typename item_type>
    bool get_property(const std::string & name, std::list<item_type> & value, bool throw_error = true) const;

    void str(json_writer & writer) const override;
    std::shared_ptr<json_value> clone(bool is_deep_clone) const override;

  private:
    friend class vjson_file_parser;
    std::list<std::shared_ptr<json_property>> properties_;
  };
  
  class json_array : public json_value
  {
  public:
    json_array();
    json_array(
      int line,
      int column);

    size_t size() const {
      return this->items_.size();
    }
    
    const std::shared_ptr<json_value> & get(size_t index) const {
      return this->items_[index];
    }

    void add(const std::shared_ptr<json_value> & item)
    {
      this->items_.push_back(item);
    }

    void str(json_writer & writer) const override;
    std::shared_ptr<json_value> clone(bool is_deep_clone) const override;

  private:
    std::vector<std::shared_ptr<json_value>> items_;
  };
  
  template<typename item_type>
  inline void json_object::add_property(const std::string & name, const std::list<item_type> & value)
  {
    auto array = std::make_shared<json_array>();
    for(auto & item : value){
      array->add(item.serialize());
    }
    
    this->add_property(name, array);
  }
  
  inline void json_object::add_property(const std::string & name, const std::list<uint16_t> & value)
  {
    auto array = std::make_shared<json_array>();
    for(auto & item : value){
      array->add(std::make_shared<json_primitive>(std::to_string(item)));
    }
    
    this->add_property(name, array);
  }

  template<typename item_type>
  inline bool json_object::get_property(const std::string & name, std::list<item_type> & value, bool throw_error) const
  {
    auto array = std::dynamic_pointer_cast<json_array>(this->get_property(name));
    if(!array){
      if(throw_error){
        throw std::runtime_error("Invalid property " + name);
      }
      
      return false;
    }
    
    for(size_t i = 0; i < array->size(); ++i) {
      value.push_back(item_type(array->get(i)));
    }
    
    return true;
  }
}

#endif // __VDS_PARSER_JSON_OBJECT_H_
