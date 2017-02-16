#ifndef __VDS_PARSER_JSON_OBJECT_H_
#define __VDS_PARSER_JSON_OBJECT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class json_writer;

  class json_value
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
    
  private:
    std::string value_;
  };
  
  class json_property : public json_value
  {
  public:
    json_property(int line, int column);
    json_property(const std::string & name, json_value * val);

    const std::string & name() const {
      return this->name_;
    }

    json_value * value() const {
      return this->value_.get();
    }

    void name(const std::string & value) {
      this->name_ = value;
    }

    void value(json_value * val) {
      this->value_.reset(val);
    }
    
    void str(json_writer & writer) const override;

  private:
    std::string name_;
    std::unique_ptr<json_value> value_;
  };
  
  class json_object : public json_value
  {
  public:
    json_object();
    json_object(int line, int column);

    void visit(const std::function<void(const json_property &)> & visitor) const;
    
    void add_property(json_property * prop);

    const json_value * get_property(const std::string & name) const;

    void str(json_writer & writer) const override;
  private:
    friend class vjson_file_parser;
    std::list<std::unique_ptr<json_property>> properties_;
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
    
    const json_value * get(int index) const {
      return this->items_[index].get();
    }

    void add(json_value * item)
    {
      this->items_.push_back(std::unique_ptr<json_value>(item));
    }
    
    void str(json_writer & writer) const override;
    
  private:
    std::vector<std::unique_ptr<json_value>> items_;
  };
  
  
}

#endif // __VDS_PARSER_JSON_OBJECT_H_
