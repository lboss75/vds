#ifndef __VRT_VARIABLE_H_
#define __VRT_VARIABLE_H_

namespace vds {
  class vrt_object;
  class vrt_variable_declaration;
  
  class vrt_variable : public std::enable_shared_from_this<vrt_variable>
  {
  public:
    vrt_variable(
      const vrt_variable_declaration * declaration,
      const std::shared_ptr<vrt_object> & value
    );

    void set_value(const std::shared_ptr<vrt_object> & value) {
      this->value_ = value;
    }

    const std::shared_ptr<vrt_object> & get_value() const {
      return this->value_;
    }

  private:
    const vrt_variable_declaration * declaration_;
    std::shared_ptr<vrt_object> value_;
  };
}

#endif // __VRT_VARIABLE_H_
