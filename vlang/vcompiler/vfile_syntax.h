#ifndef __VFILE_SYNTAX_H_
#define __VFILE_SYNTAX_H_

#include <string>
#include <list>
#include "vlex.h"
#include "vjson_object.h"
#include "vjson_file_parser.h"

namespace vds {
  class vclass;
  class vfile;
  class vlexer;
  
  struct vpackage
  {
    vpackage();
    ~vpackage();

    vlex name;
    std::shared_ptr<vjson_object> properties;
  };
  
  class vlexer_object
  {
  public:
    vlexer_object(
      const vfile & owner,
      int line,
      int column      
    );
    virtual ~vlexer_object();
    
    const vfile & owner() const
    {
      return this->owner_;
    }
    
    int line() const
    {
      return this->line_;
    }
    
    int column() const
    {
      return this->column_;
    }
    
  private:
    const vfile & owner_;
    int line_;
    int column_;
  };
  
  
  class vexpression : public vlexer_object
  {
  public:
    vexpression(
      const vfile & owner,
      int line,
      int column      
    );
    
  };
  
  class vname_expression : public vexpression
  {
  public:
    vname_expression(
      const vfile & owner,
      int line,
      int column,
      const std::string & name);
    
    const std::string & name() const
    {
      return this->name_;
    }
    
  private:
    std::string name_;
  };

  class vnumber_expression : public vexpression
  {
  public:
    vnumber_expression(
      const vfile & owner,
      int line,
      int column,
      const std::string & value);

    const std::string & value() const
    {
      return this->value_;
    }
    
  private:
    std::string value_;
  };
  
  class vstring_expression : public vexpression
  {
  public:
    vstring_expression(
      const vfile & owner,
      int line,
      int column,
      const std::string & value);

    const std::string & value() const
    {
      return this->value_;
    }
    
  private:
    std::string value_;
  };

  class vproperty_expression : public vexpression
  {
  public:
    vproperty_expression(
      vexpression * object,
      const vfile & owner,
      int line,
      int column,
      const std::string & name);
    
    const vexpression * object () const
    {
      return this->object_.get();
    }
    
    const std::string & name() const
    {
      return this->name_;
    }
    
  private:
    std::string name_;
    std::unique_ptr<vexpression> object_;
  };

  class vbinary_exptession : public vexpression
  {
  public:
    vbinary_exptession(
      const vfile & owner,
      int line,
      int column,
      const std::string & op,
      vexpression * left,
      vexpression * right);

    const vexpression * left() const
    {
      return this->left_.get();
    }
    
    const vexpression * right() const
    {
      return this->right_.get();
    }
    
    const std::string & op() const
    {
      return this->op_;
    }
    
  private:
    std::string op_;
    std::unique_ptr<vexpression> left_;
    std::unique_ptr<vexpression> right_;
  };
  
  class vinvoke_argument : public vlexer_object
  {
  public:
    vinvoke_argument(
      const vfile & owner,
      int line,
      int column,
      const std::string & name,
      vexpression * value
    );

    const std::string & name() const
    {
      return this->name_;
    }

    const vexpression * value() const
    {
      return this->value_.get();
    }

  private:
    std::string name_;
    std::unique_ptr<vexpression> value_;
  };
  
  class vinvoke_expression : public vexpression
  {
  public:
    vinvoke_expression(
      vexpression * object,
      const vfile & owner,
      int line,
      int column,
      const std::string & name
    );
    const vexpression * object() const {
      return this->object_.get();
    }

    const std::string & name() const {
      return this->name_;
    }

    const std::list<std::unique_ptr<vinvoke_argument>> & arguments() const {
      return this->arguments_;
    }

  private:
    std::unique_ptr<vexpression> object_;
    std::string name_;
    std::list<std::unique_ptr<vinvoke_argument>> arguments_;
    
    friend class vfile_syntax;
  };
  
  class vtype : public vlexer_object
  {
  public:
    vtype(
      const vfile & owner,
      int line,
      int column,
      const std::string & name);
    
    const std::string name() const
    {
      return this->name_;
    }

  private:
    int array_rank_;
    bool is_optional_;
    std::string name_;
    
    friend class vfile_syntax;
  };

  class vnew_object_expression : public vexpression
  {
  public:
    vnew_object_expression(
      vtype * type
    );
    
    const vtype * type() const
    {
      return this->type_.get();
    }

    const std::list<std::unique_ptr<vinvoke_argument>> & arguments() const
    {
      return this->arguments_;
    }

  private:
    std::unique_ptr<vtype> type_;
    std::list<std::unique_ptr<vinvoke_argument>> arguments_;
    
    friend class vfile_syntax;
  };
 
  class vstatement : public vlexer_object
  {
  public:
    vstatement(
      const vds::vfile& owner,
      int line,
      int column);
  };
  class vblock_statement : public vstatement
  {
  public:
    vblock_statement(
      const vfile & owner,
      int line,
      int column);
    
    const std::list<std::unique_ptr<vstatement>> & statements() const
    {
      return this->statements_;
    }
    
  private:
    std::list<std::unique_ptr<vstatement>> statements_;
    
    friend class vfile_syntax;
  };
  
  class vexpression_statement : public vstatement
  {
  public:
    vexpression_statement(      
      const vfile & owner,
      int line,
      int column);
    
    vexpression_statement(
      vexpression * expression);
    
    const vexpression * expression() const
    {
      return this->expression_.get();
    }
    
  private:
    std::unique_ptr<vexpression> expression_;
    
    friend class vfile_syntax;
  };
  
  class vfor_statement : public vstatement
  {
  public:
    vfor_statement(
      const vfile & owner,
      int line,
      int column      
    );
    
  private:
    std::unique_ptr<vstatement> init_;
    std::unique_ptr<vexpression> condition_;
    std::unique_ptr<vstatement> step_;
    std::unique_ptr<vblock_statement> body_;
    
    friend class vfile_syntax;
  };

  class vforeach_statement : public vstatement
  {
  public:
    vforeach_statement(
      const vfile & owner,
      int line,
      int column      
    );
    
  private:
    std::string variable_;
    std::unique_ptr<vexpression> collection_;
    std::unique_ptr<vblock_statement> body_;
    
    friend class vfile_syntax;
  };

  class vif_statement : public vstatement
  {
  public:
    vif_statement(
      const vfile & owner,
      int line,
      int column      
    );
    
  private:
    std::unique_ptr<vexpression> condition_;
    std::unique_ptr<vblock_statement> body_;
    std::unique_ptr<vblock_statement> else_body_;
    
    friend class vfile_syntax;
  };
  
  class vreturn_statement : public vstatement
  {
  public:
    vreturn_statement(
      const vfile & owner,
      int line,
      int column,
      vexpression * body);
    
    const vexpression * body() const {
      return this->body_.get();
    }

  private:
    std::unique_ptr<vexpression> body_;
    
    friend class vfile_syntax;
  };
  
  class var_statement : public vstatement
  {
  public:
    var_statement(
      const vfile & owner,
      int line,
      int column,
      const std::string & name);
    
    const vtype * type() const {
      return this->type_.get();
    }
    const std::string & name() const
    {
      return this->name_;
    }
    
    const vexpression * init_value() const
    {
      return this->init_value_.get();
    }
    
  private:
    std::unique_ptr<vtype> type_;
    std::string name_;
    std::unique_ptr<vexpression> init_value_;
    
    friend class vfile_syntax;
  };
  
  class vmethod_parameter : public vlexer_object
  {
  public:
    vmethod_parameter(
      const vfile & owner,
      int line,
      int column,
      const std::string & name);

    const vtype * type() const {
      return this->type_.get();
    }
    
    const std::string name() const
    {
      return this->name_;
    }
    
  private:
    std::string name_;
    std::unique_ptr<vtype> type_;
    
    friend class vfile_syntax;
  };
  
  class vmethod : public vlexer_object
  {
  public:
    vmethod(
      const vfile & owner,
      int line,
      int column,
      const vclass * cls,
      const std::string & name);
    
    const std::string & name() const
    {
      return this->name_;
    }
    const std::list<std::unique_ptr<vmethod_parameter>> & parameters() const
    {
      return this->parameters_;
    }
    
    const vblock_statement *  body() const
    {
      return this->body_.get();
    }
    
    const vtype * result_type() const
    {
      return this->result_type_.get();
    }
    
    const vds::vclass * declaring_type() const {
      return this->declaring_type_;
    }
    
    bool is_external() const {
      return this->is_external_;
    }
    
    bool is_static() const {
      return this->is_static_;
    }
    
  private:
    const vds::vclass * declaring_type_;
    std::string name_;
    std::list<std::unique_ptr<vmethod_parameter>> parameters_;
    std::unique_ptr<vblock_statement> body_;
    std::unique_ptr<vtype> result_type_;
    bool is_public_;
    bool is_external_;
    bool is_static_;
    bool is_override_;
    
    friend class vfile_syntax;
  };
  
  class vproperty_verb : public vlexer_object
  {
  public:
    vproperty_verb(
      const vfile & owner,
      int line,
      int column,
      const std::string & name);
    vproperty_verb(
      const vfile & owner,
      int line,
      int column,
      const std::string & name,
      vblock_statement * block);
  private:
    std::string name_;
    std::unique_ptr<vblock_statement> block_;
  };
  
  class vproperty : public vlexer_object
  {
  public:
    vproperty(
      const vfile & owner,
      int line,
      int column,
      bool is_dependency,
      const std::string & name);
    
    const std::string & name() const
    {
      return this->name_;
    }
    
    const vtype * result_type() const
    {
      return this->result_type_.get();
    }
    
    const std::map<std::string, std::unique_ptr<vproperty_verb>> & verbs() const {
      return this->verbs_;
    }

    bool is_dependency() const {
      return this->is_dependency_;
    }

  private:
    std::string name_;
    std::unique_ptr<vexpression> init_;
    std::unique_ptr<vtype> result_type_;
    std::map<std::string, std::unique_ptr<vproperty_verb>> verbs_;
    bool is_dependency_;
    bool is_public_;
    bool is_external_;
    bool is_static_;
    bool is_override_;
    
    friend class vfile_syntax;
  };

  class vlambda_expression : public vexpression
  {
  public:
    vlambda_expression(
      const vfile & owner,
      int line,
      int column);

  private:
    std::list<std::unique_ptr<vmethod_parameter>> parameters_;
    std::unique_ptr<vblock_statement> body_;
    std::unique_ptr<vtype> result_type_;
    
    friend class vfile_syntax;
  };
  
  class vbase_class : public vlexer_object
  {
  public:
    vbase_class(
      const vfile & owner,
      int line,
      int column,
      const std::string & name);
    
  private:
    std::string name_;
  };
  
  class vclass : public vlexer_object
  {
  public:
    vclass(
      const vfile & owner,
      int line,
      int column,
      bool is_interface,
      const std::string & name);
    
    bool is_interface() const
    {
      return this->is_interface_;
    }
    
    const std::string & name() const
    {
      return this->name_;
    }
    
    const std::list<std::unique_ptr<vmethod>> & methods() const
    {
      return this->methods_;
    }
    
    const std::list<std::unique_ptr<vproperty>> & properties() const
    {
      return this->properties_;
    }
    
  private:
    bool is_interface_;
    std::string name_;
    std::list<std::unique_ptr<vmethod>> methods_;
    std::list<std::unique_ptr<vproperty>> properties_;
    std::list<std::unique_ptr<vbase_class>> base_classes_;
    
    friend class vfile_syntax;
  };
  
  class vnamespace : public vlexer_object
  {
  public:
    vnamespace(
      const vfile & owner,
      int line,
      int column,
      const std::string & name);
    
    const std::string & name() const
    {
      return this->name_;
    }      
    
    const std::list<std::unique_ptr<vclass>> & classes() const
    {
      return this->classes_;
    }
    
  private:
    std::string name_;
    std::list<std::unique_ptr<vclass>> classes_;
    
    friend class vfile_syntax;
  };
  
  class vfile
  {
  public:
    vfile(const std::string & file_path);
    ~vfile();

    const std::string & file_path() const
    {
      return this->file_path_;
    }
    
    const vpackage * package() const
    {
      return this->package_.get();
    }
    
    const std::list<std::unique_ptr<vnamespace>> & namespaces() const
    {
      return this->namespaces_;
    }
    
    const std::list<vlex> & usings() const
    {
      return this->usings_;
    }
    
  private:
    std::string file_path_;
    std::unique_ptr<vpackage> package_;
    std::list<std::unique_ptr<vnamespace>> namespaces_;
    std::list<vlex> usings_;
    
    friend class vpackage_compiler;
    friend class vfile_syntax;
  };

  
  class vfile_syntax : private vjson_file_parser
  {
  public:    
    static std::unique_ptr<vfile> parse(vlexer & lexer);
    
  private:
    vfile_syntax(vlexer & lexer);
    
    std::unique_ptr<vfile> file_;
    
    void parse_file();
    void parse_package();
    
    vlex parse_name(bool allow_dot = true);
    
    void parse_using();
    void parse_namespace();
    
    void parse_namespace_body(const std::unique_ptr<vnamespace> & ns);
    void parse_class_body(const std::unique_ptr<vclass> & cls);
    
    void parse_method(const std::unique_ptr<vmethod> & m);
    void parse_method_parameter(const std::unique_ptr<vmethod_parameter> & p);
    std::unique_ptr<vblock_statement> parse_method_body(const vlex & body_start);
    void parse_block_statement(const std::unique_ptr<vblock_statement> & block);
    std::unique_ptr<vblock_statement> parse_block_statement();

    void parse_property(const std::unique_ptr<vproperty> & p);
    
    std::unique_ptr<vstatement> parse_statement(const char * final_operator = ";");
    
    std::unique_ptr<vfor_statement> parse_for(const vlex & operator_start);
    std::unique_ptr<vforeach_statement> parse_foreach(const vlex & operator_start);
    std::unique_ptr<var_statement> parse_var(const vlex & operator_start);
    std::unique_ptr<vif_statement> parse_if(const vlex & operator_start);
    std::unique_ptr<vreturn_statement> parse_return(const vlex & operator_start);
    
    std::unique_ptr<vexpression> parse_expression();

    std::unique_ptr<vexpression> parse_expression0();
    std::unique_ptr<vexpression> parse_expression1();
    std::unique_ptr<vexpression> parse_expression2();
    std::unique_ptr<vexpression> parse_expression3();
    std::unique_ptr<vexpression> parse_expression4();
    std::unique_ptr<vexpression> parse_expression5();
    std::unique_ptr<vexpression> parse_expression6();
    std::unique_ptr<vexpression> parse_expression7();
    std::unique_ptr<vexpression> parse_expression8();
    std::unique_ptr<vexpression> parse_expression9();
    std::unique_ptr<vexpression> parse_expression10();
    std::unique_ptr<vexpression> parse_expression11();
    std::unique_ptr<vexpression> parse_expression12();
    std::unique_ptr<vexpression> parse_expression13();
    std::unique_ptr<vexpression> parse_expression14();
    std::unique_ptr<vexpression> parse_expression15();
    std::unique_ptr<vexpression> parse_expression16();
    
    void parse_lambda(const std::unique_ptr<vlambda_expression> &);

    void parse_invoke_arguments(std::list<std::unique_ptr<vinvoke_argument>> & arguments);
    std::unique_ptr<vinvoke_argument> parse_invoke_argument();
    
    std::unique_ptr<vtype> parse_type();
  };
}

#endif // __VFILE_SYNTAX_H_
