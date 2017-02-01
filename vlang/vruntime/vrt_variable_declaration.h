#ifndef __VRT_VARIABLE_DECLARATION_H_
#define __VRT_VARIABLE_DECLARATION_H_

namespace vds {
  class vrt_method;
  class vrt_source_file;
  class vrt_type;

  class vrt_variable_declaration
  {
  public:
    vrt_variable_declaration(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_type * type,
      const std::string & name
    );

    const std::string & name() const {
      return this->name_;
    }

    const vrt_type * type() const {
      return this->type_;
    }

    const vrt_method * init_value() const;

  private:
    const vrt_source_file * file_;
    int line_;
    int column_;
    std::string name_;
    const vrt_type * type_;

    std::unique_ptr<vrt_method> init_value_;
  };
}

#endif // __VRT_VARIABLE_H_
