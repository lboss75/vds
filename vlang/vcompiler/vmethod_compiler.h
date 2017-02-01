#ifndef __VMETHOD_COMPILER_H_
#define __VMETHOD_COMPILER_H_

namespace vds {
  class name_resolver;
  class vclass;
  class vmethod;
  class var_statement;
  class vexpression;
  class vblock_statement;
  class vname_expression;
  class vnew_object_expression;
  class vproperty_expression;
  class vrt_callable;
  class vrt_expression;
  class vrt_method;
  class vrt_statement;
  class vrt_type;
  class vruntime_machine;
  class vreturn_statement;
  class vstatement;
  class vtype;
  class vtype_resolver;
  
  class vmethod_compiler
  {
  public:
    vmethod_compiler(
      vtype_resolver * resolver,
      vrt_method * target);
    
    void compile(vmethod * source);
    void generate_primitive_constructor(vclass * cls);

  private:    
    struct variable_info
    {
      size_t index;
      const vrt_type * type;
    };
    
    vtype_resolver * resolver_;
    vrt_method * target_;
    std::map<std::string, variable_info> variables_;
    friend class vpackage_compiler;
    
    void compile_block(
      const vblock_statement * statement);
    void compile(
      const vstatement * statement);
    
    void compile_var_statement(
      const var_statement * st);

    void compile_return_statement(
      const vreturn_statement * st);

    
    const vrt_type * compile_expression(
      const vexpression * ex);
    
    const vrt_type * compile_property(
      const vproperty_expression * ex);

    const vrt_type * compile_name(
      const vname_expression * ex);

    const vrt_type * compile_new_object_expression(
      const vnew_object_expression * ex);
    
    size_t register_variable(
      int line,
      int column,
      const vrt_type * type,
      const std::string & name);
    
    const vrt_property * resolve_property(const vrt_type * type, const std::string & name);
    
    void push(vrt_statement * st) const;
  };
}

#endif // !__VMETHOD_COMPILER_H_
