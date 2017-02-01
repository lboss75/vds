#ifndef __VRT_EXPRESSION_H_
#define __VRT_EXPRESSION_H_

#include "vrt_statement.h"

namespace vds {
  class vrt_callable;
  class vrt_context;
  class vrt_constructor;
  class vrt_property;
  class vrt_source_file;
  class vrt_type;
  
  class vrt_expression : public vrt_statement
  {
  public:
    vrt_expression(
      const vrt_source_file * file,
      int line,
      int column);
  };
  
  class vrt_new_object_expression : public vrt_expression
  {
  public:
    vrt_new_object_expression(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_constructor * constructor,
      const std::list<std::string> & arguments
    );
    
    bool execute(
      vrt_context & context) const override;

  private:
    const vrt_constructor * constructor_;
    std::list<std::string> arguments_;
  };
  
  class vrt_type_reference : public vrt_expression
  {
  public:
    vrt_type_reference(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_type * type);
  private:
    const vrt_type * type_;
  };
  
  class vrt_localvariable_reference : public vrt_expression
  {
  public:
    vrt_localvariable_reference(
      const vrt_source_file * file,
      int line,
      int column,
      size_t index);
    
    bool execute(
      vrt_context & context) const override;
  private:
    size_t index_;
  };
  
  class vrt_get_property  : public vrt_expression
  {
  public:
    vrt_get_property(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_property * property);
    
    bool execute(
      vrt_context & context) const override;
    
  private:
    const vrt_property * property_;
  };

  class vrt_string_const  : public vrt_expression
  {
  public:
    vrt_string_const(
      const vrt_source_file * file,
      int line,
      int column,
      const std::string & value);
    
    bool execute(
      vrt_context & context) const override;
    
  private:
    std::string value_;
  };

  class vrt_number_const  : public vrt_expression
  {
  public:
    vrt_number_const(
      const vrt_source_file * file,
      int line,
      int column,
      const std::string & value);
    
    bool execute(
      vrt_context & context) const override;
    
  private:
    std::string value_;
  };

  class vrt_method_invoke : public vrt_expression
  {
  public:
    vrt_method_invoke(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_callable * method,
      const std::list<std::string> & arguments
    );

    bool execute(
      vrt_context & context) const override;

  private:
    const vrt_callable * method_;
    std::list<std::string> arguments_;
  };

}

#endif // ! __VRT_EXPRESSION_H_
