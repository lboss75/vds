#ifndef __VRT_JSON_H_
#define __VRT_JSON_H_

namespace vds {
  class vrt_source_file;

  class vrt_json_value
  {
  public:
    vrt_json_value(
      const vrt_source_file * source_file,
      int line,
      int column);

    virtual ~vrt_json_value();

    const vrt_source_file * source_file() const {
      return this->source_file_;
    }

    int line() const {
      return this->line_;
    }

    int column() const {
      return this->column_;
    }

  private:
    const vrt_source_file * source_file_;
    int line_;
    int column_;
  };

  class vrt_json_primitive : public vrt_json_value
  {
  public:
    vrt_json_primitive(
      const vrt_source_file * source_file,
      int line,
      int column,
      const  std::string & value);

    const std::string & value() const {
      return this->value_;
    }

  private:
    std::string value_;
  };

  struct vrt_json_property
  {
    std::string name;
    std::unique_ptr<vrt_json_value> value;
  };

  class vrt_json_object : public vrt_json_value
  {
  public:
    vrt_json_object(
      const vrt_source_file * source_file,
      int line,
      int column);

    void visit(const std::function<void(const vrt_json_property &)> & visitor) const;

  private:
    friend class vpackage_compiler;
    std::list<vrt_json_property> properties_;
  };
  
  class vrt_json_array : public vrt_json_value
  {
  public:
    vrt_json_array(
      const vrt_source_file * source_file,
      int line,
      int column);

    size_t size() const {
      return this->items_.size();
    }
    
    const vrt_json_value * get(int index) const {
      return this->items_[index].get();
    }

  private:
    friend class vpackage_compiler;
    std::vector<std::unique_ptr<vrt_json_value>> items_;
  };
}

#endif//__VRT_JSON_H_