#ifndef __VDS_PARSER_JSON_OBJECT_H_
#define __VDS_PARSER_JSON_OBJECT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  
  class json_value
  {
  public:
    json_value(int line, int column);
    virtual ~json_value();

    int line() const {
      return this->line_;
    }
    
    int column() const {
      return this->column_;
    }
    
  private:
    int line_;
    int column_;
  };
  
  class json_primitive : public json_value
  {
  public:
    json_primitive(
      int line, int column,
      const std::string & value);
    
    const std::string & value() const {
      return this->value_;
    }
    
  private:
    std::string value_;
  };
  
  struct json_property
  {
    int line;
    int column;
    std::string name;
    std::unique_ptr<json_value> value;
  };
  
  class json_object : public json_value
  {
  public:
    json_object(int line, int column);

    void visit(const std::function<void(const json_property &)> & visitor) const;
    
  private:
    friend class vjson_file_parser;
    std::list<json_property> properties_;
  };
  
  class json_array : public json_value
  {
  public:
    json_array(
      int line,
      int column);

    size_t size() const {
      return this->items_.size();
    }
    
    const json_value * get(int index) const {
      return this->items_[index].get();
    }

    void add(json_value * item)
    {
      this->items_.push_back(std::unique_ptr<json_value>(item));
    }
    
  private:
    std::vector<std::unique_ptr<json_value>> items_;
  };
  
}

#endif // __VDS_PARSER_JSON_OBJECT_H_
