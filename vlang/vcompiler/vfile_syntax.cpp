#include "stdafx.h"
#include "vfile_syntax.h"
#include "vlexer.h"
#include "compile_error.h"

vds::vlexer_object::vlexer_object(
  const vds::vfile& owner, int line, int column)
: owner_(owner), line_(line), column_(column)
{
}

vds::vlexer_object::~vlexer_object()
{
}

vds::vstatement::vstatement(const vds::vfile& owner, int line, int column)
: vlexer_object(owner, line, column)
{
}

vds::vexpression::vexpression(
  const vds::vfile& owner, int line, int column)
: vlexer_object(owner, line, column)
{
}

vds::vexpression_statement::vexpression_statement(const vds::vfile& owner, int line, int column)
: vstatement(owner, line, column)
{
}

vds::vforeach_statement::vforeach_statement(const vds::vfile& owner, int line, int column)
: vstatement(owner, line, column)
{
}

vds::vif_statement::vif_statement(const vds::vfile& owner, int line, int column)
: vstatement(owner, line, column)
{
}

vds::vexpression_statement::vexpression_statement(
  vexpression * expression)
: vstatement(expression->owner(), expression->line(), expression->column()),
expression_(expression)
{

}


vds::vblock_statement::vblock_statement(
  const vds::vfile& owner, int line, int column)
: vstatement(owner, line, column)
{
}

vds::vfor_statement::vfor_statement(const vds::vfile& owner, int line, int column)
: vstatement(owner, line, column)
{
}

vds::vfile_syntax::vfile_syntax(vds::vlexer& lexer)
: vds::vjson_file_parser(lexer),
file_(new vfile(lexer.filename()))
{
}

std::unique_ptr<vds::vfile> vds::vfile_syntax::parse(vlexer & lexer)
{
  vfile_syntax compiler(lexer);
  while (LT_EOF != compiler.next_lex_.type) {
    if (LT_KEYWORD == compiler.next_lex_.type) {
      if ("package" == compiler.next_lex_.token.value) {
        if (nullptr != compiler.file_->package_) {
          throw new compile_error(
            compiler.lexer_.filename(),
            compiler.next_lex_.token.line,
            compiler.next_lex_.token.column,
            "package already defined");
        }

        compiler.parse_package();
        continue;
      }

      if ("using" == compiler.next_lex_.token.value) {
        compiler.parse_using();
        continue;
      }
      
      if ("namespace" == compiler.next_lex_.token.value) {
        compiler.parse_namespace();
        continue;
      }
    }
    
    compiler.unexpected();
  }
  
  return std::move(compiler.file_);
}

void vds::vfile_syntax::parse_using()
{
  this->next();
  this->file_->usings_.push_back(this->parse_name());
  this->require(";");
}

void vds::vfile_syntax::parse_namespace()
{
  this->next();
  
  auto name = this->parse_name();

  this->require("{");
    
  std::unique_ptr<vnamespace> result(new vnamespace(
    *this->file_.get(),
    name.token.line,
    name.token.column,
    name.token.value));
  this->parse_namespace_body(result);
  this->file_->namespaces_.push_back(std::move(result));
}

void vds::vfile_syntax::parse_namespace_body(const std::unique_ptr<vnamespace> & ns)
{
  for(;;) {
    auto is_public = this->optional("public", LT_KEYWORD);
    
    bool is_class = this->optional("class", LT_KEYWORD);
    bool is_interface = (!is_class) && this->optional("interface", LT_KEYWORD);
    
    if (is_class || is_interface) {
      auto name = this->parse_name();
      std::unique_ptr<vclass> cls(new vclass(
        *this->file_.get(),
        name.token.line,
        name.token.column,
        is_interface,
        name.token.value));
      this->parse_class_body(cls);
      ns->classes_.push_back(std::move(cls));
      continue;
    }
    
    this->require("}");
    break;
  }
}


void vds::vfile_syntax::parse_package()
{
  this->next();
  this->file_->package_.reset(new vpackage());
  this->file_->package_->name = this->parse_name();
  this->require("{");
  this->file_->package_->properties = this->parse_json_object();
}

vds::vlex vds::vfile_syntax::parse_name(bool allow_dot)
{
  if (LT_NAME != this->next_lex_.type) {
    throw new compile_error(
      this->lexer_.filename(),
      this->next_lex_.token.line,
      this->next_lex_.token.column,
      "Name expected");
  }
  vlex result = this->next_lex_;
  this->next();
  
  if (allow_dot) {
    for(;;){
      if (LT_OPERATOR == this->next_lex_.type
        && "." == this->next_lex_.token.value
      ) {
        this->next();
        if (LT_NAME != this->next_lex_.type) {
          throw new compile_error(
            this->lexer_.filename(),
            this->next_lex_.token.line,
            this->next_lex_.token.column,
            "Name expected");
        }
        
        result.token.value += ".";
        result.token.value += this->next_lex_.token.value;
        this->next();
     }
      else {
        break;
      }
    }
  }
  
  return result;
}



vds::vfile::vfile(const std::string & file_path)
  : file_path_(file_path)
{
}

vds::vfile::~vfile()
{
}

vds::vpackage::vpackage()
//  : properties(nullptr)
{
}

vds::vpackage::~vpackage()
{
}

vds::vnamespace::vnamespace(const vds::vfile& owner, int line, int column, const std::string& name)
: vlexer_object(owner, line, column), name_(name)
{
}


vds::vclass::vclass(
  const vds::vfile& owner,
  int line,
  int column,
  bool is_interface,
  const std::string& name)
: vlexer_object(owner, line, column),
is_interface_(is_interface), name_(name)
{
}

void vds::vfile_syntax::parse_class_body(
  const std::unique_ptr<vds::vclass> & cls)
{
  if(this->optional(":")){
    do{
      auto name = this->parse_name();
      cls->base_classes_.push_back(
        std::unique_ptr<vbase_class>(
          new vbase_class(
            *this->file_.get(),
            name.token.line,
            name.token.column,
            name.token.value)));
    } while(this->optional(","));
  }
  
  this->require("{");
  while(!this->optional("}")) {
    if (this->optional("var", LT_KEYWORD)) {
      auto var_name = this->parse_name(false);
      //TODO
    }

    auto is_public_member = this->optional("public", LT_KEYWORD);
    auto is_external_member = this->optional("external", LT_KEYWORD);
    auto is_static_member = this->optional("static", LT_KEYWORD);
    auto is_override = this->optional("override", LT_KEYWORD);
    
    auto is_dependency = this->optional("dependency", LT_KEYWORD);
    auto is_property = (!is_dependency) && this->optional("property", LT_KEYWORD);
    
    auto name = this->parse_name();
    
    if (is_dependency || is_property) {
      std::unique_ptr<vproperty> property(new vproperty(
        *this->file_.get(),
        name.token.line,
        name.token.column,
        is_dependency,
        name.token.value));
      property->is_public_ = is_public_member;
      property->is_external_ = is_external_member;
      property->is_static_ = is_static_member;
      property->is_override_ = is_override;
      this->parse_property(property);
      cls->properties_.push_back(std::move(property));
    } else {
      std::unique_ptr<vmethod> method(new vmethod(
        *this->file_.get(),
        name.token.line,
        name.token.column,
        cls.get(),
        name.token.value));
      method->is_public_ = is_public_member;
      method->is_external_ = is_external_member;
      method->is_static_ = is_static_member;
      method->is_override_ = is_override;
      this->parse_method(method);
      cls->methods_.push_back(std::move(method));
    }
  }
}

vds::vmethod::vmethod(
  const vfile & owner,
  int line,
  int column,
  const vclass * cls,
  const std::string & name)
: vlexer_object(owner, line, column),
  declaring_type_(cls), name_(name),
  is_public_(false), is_external_(false), is_static_(false),
  is_override_(false)
{
}

vds::vproperty::vproperty(
  const vds::vfile& owner,
  int line,
  int column,
  bool is_dependency,
  const std::string& name)
: vlexer_object(owner, line, column), name_(name),
  is_dependency_(is_dependency),
  is_public_(false), is_external_(false), is_static_(false),
  is_override_(false)
{

}


void vds::vfile_syntax::parse_method(
  const std::unique_ptr<vds::vmethod> & m)
{
  this->require("(");
  for(;;) {
    if(this->optional(")")) {
      break;
    }
        
    if (LT_NAME == this->next_lex_.type) {
      std::unique_ptr<vmethod_parameter> parameter(new vmethod_parameter(
        *this->file_.get(),
        this->next_lex_.token.line,
        this->next_lex_.token.column,
        this->next_lex_.token.value));
      this->next();
      this->parse_method_parameter(parameter);
      m->parameters_.push_back(std::move(parameter));
      
      if(this->optional(")")) {
        break;
      }
      
      if(this->optional(",")) {
        continue;
      }
    }
    
    this->unexpected();
  }
  
  if (this->optional(":")) {
    m->result_type_ = std::move(this->parse_type());
  }

  if(m->is_external_){
    this->require(";", LT_OPERATOR);
  } else {
    if(!m->declaring_type()->is_interface() || !this->optional(";", LT_OPERATOR)) {
      vlex body_start;
      this->require("{", LT_OPERATOR, &body_start);
      m->body_ = std::move(this->parse_method_body(body_start));
    }
  }
}

void vds::vfile_syntax::parse_lambda(
  const std::unique_ptr<vds::vlambda_expression> & result)
{
  this->require("(");
  for(;;) {
    if(this->optional(")")) {
      break;
    }
        
    if (LT_NAME == this->next_lex_.type) {
      std::unique_ptr<vmethod_parameter> parameter(new vmethod_parameter(
        *this->file_.get(),
        this->next_lex_.token.line,
        this->next_lex_.token.column,
        this->next_lex_.token.value));
      this->next();
      this->parse_method_parameter(parameter);
      result->parameters_.push_back(std::move(parameter));
      
      if(this->optional(")")) {
        break;
      }
      
      if(this->optional(",")) {
        continue;
      }
    }
    
    this->unexpected();
  }
  
  if (this->optional(":")) {
    result->result_type_ = this->parse_type();
  }
  
  vlex body_start;
  this->require("{", LT_OPERATOR, &body_start);
  result->body_ = this->parse_method_body(body_start);

}


vds::vmethod_parameter::vmethod_parameter(
  const vfile & owner,
  int line,
  int column,
  const std::string & name)
: vlexer_object(owner, line, column), name_(name)
{
}

void vds::vfile_syntax::parse_method_parameter(
  const std::unique_ptr<vds::vmethod_parameter> & p)
{
  this->require(":");
  p->type_ = this->parse_type();
}

vds::vtype::vtype(
  const vfile & owner,
  int line,
  int column,
  const std::string & name)
: vlexer_object(owner, line, column), name_(name), array_rank_(0), is_optional_(false)
{
}

std::unique_ptr<vds::vtype> vds::vfile_syntax::parse_type()
{
  auto name = this->parse_name();
  std::unique_ptr<vds::vtype> result(new vtype(
    *this->file_.get(),
    name.token.line,
    name.token.column,
    name.token.value
  ));

  while (this->optional("[")) {
    this->require("]");
    result->array_rank_++;
  }

  if (this->optional("?")) {
    result->is_optional_ = true;
  }

  return result;
}

std::unique_ptr<vds::vblock_statement> vds::vfile_syntax::parse_method_body(const vlex & body_start)
{
  std::unique_ptr<vblock_statement> result(new vblock_statement(
    *this->file_.get(),
    body_start.token.line,
    body_start.token.column
  ));
  
  this->parse_block_statement(result);
  return result;
}

std::unique_ptr<vds::vblock_statement> vds::vfile_syntax::parse_block_statement()
{
  vlex start_block;
  this->require("{", LT_OPERATOR, &start_block);
  
  std::unique_ptr<vblock_statement> result(new vblock_statement(
    *this->file_.get(),
    start_block.token.line,
    start_block.token.column
  ));
  
  this->parse_block_statement(result);
  return result;
}


void vds::vfile_syntax::parse_block_statement(
  const std::unique_ptr<vds::vblock_statement> & block)
{
  for(;;) {
    if (this->optional("}")) {
      break;
    }
    
    vlex start_operator;
    
    if (this->optional("for", LT_KEYWORD, &start_operator)) {
      block->statements_.push_back(this->parse_for(start_operator));
      continue;
    }

    if (this->optional("foreach", LT_KEYWORD, &start_operator)) {
      block->statements_.push_back(this->parse_foreach(start_operator));
      continue;
    }

    if(this->optional("var", LT_KEYWORD, &start_operator)) {
      block->statements_.push_back(this->parse_var(start_operator));
      continue;
    }
      
    if(this->optional("if", LT_KEYWORD, &start_operator)) {
      block->statements_.push_back(this->parse_if(start_operator));
      continue;
    }
    
    if(this->optional("return", LT_KEYWORD, &start_operator)) {
      block->statements_.push_back(this->parse_return(start_operator));
      continue;
    }
    
    block->statements_.push_back(this->parse_statement());
  }
}

std::unique_ptr<vds::vfor_statement> vds::vfile_syntax::parse_for(const vlex & operator_start)
{
  std::unique_ptr<vds::vfor_statement> result(new vfor_statement(
    *this->file_.get(),
    operator_start.token.line,
    operator_start.token.column
  ));
  this->require("(");
  result->init_ = this->parse_statement();
  result->condition_ = this->optional(";") ? nullptr : this->parse_expression();
  result->step_ = this->parse_statement(")");
  
  result->body_ = this->parse_block_statement();
  
  return result;
}

std::unique_ptr<vds::vforeach_statement> vds::vfile_syntax::parse_foreach(const vlex & operator_start)
{
  std::unique_ptr<vds::vforeach_statement> result(new vforeach_statement(
    *this->file_.get(),
    operator_start.token.line,
    operator_start.token.column
  ));
  this->require("(");
  this->require("var", LT_KEYWORD);
  result->variable_ = this->parse_name(false).token.value;
  this->require("in", LT_KEYWORD);
  result->collection_ = this->parse_expression();
  this->require(")");

  result->body_ = this->parse_block_statement();

  return result;
}

std::unique_ptr<vds::vstatement> vds::vfile_syntax::parse_statement(const char * final_operator)
{
  vlex operator_start;
  if(this->optional("{", LT_OPERATOR, &operator_start)) {
    std::unique_ptr<vblock_statement> block(new vblock_statement(
      *this->file_.get(),
      operator_start.token.line,
      operator_start.token.column
    ));
    
    this->parse_block_statement(block);
    return std::unique_ptr<vds::vstatement>(block.release());
  }
  
  if (this->optional(final_operator, LT_OPERATOR, &operator_start)) {
    std::unique_ptr<vexpression_statement> result(
      new vexpression_statement(
        *this->file_.get(),
        operator_start.token.line,
        operator_start.token.column
    ));
    return std::unique_ptr<vstatement>(result.release());
  }
  
  std::unique_ptr<vexpression_statement> result(
    new vexpression_statement(
      this->parse_expression().release()));
  this->require(final_operator);
  return std::unique_ptr<vstatement>(result.release());
}

vds::var_statement::var_statement(
  const vfile & owner,
  int line,
  int column,
  const std::string & name)
  : vstatement(owner, line, column),
  name_(name)
{
}



std::unique_ptr<vds::var_statement> vds::vfile_syntax::parse_var(const vlex & operator_start)
{
  auto name = this->parse_name();
  std::unique_ptr<vds::var_statement> result(new var_statement(
    *this->file_.get(),
    name.token.line,
    name.token.column,
    name.token.value
  ));
  if (this->optional(":")) {
    result->type_ = this->parse_type();
  }
  if(this->optional("=")) {
    result->init_value_ = this->parse_expression();
    this->require(";");
  };
  
  return result;
  
}

void vds::vfile_syntax::parse_property(const std::unique_ptr<vds::vproperty> & p)
{
  if(this->optional(":")){
    p->result_type_ = this->parse_type();
  }
  this->require("{");
  while(!this->optional("}")){
    auto name = this->parse_name();
    vproperty_verb * verb;
    if(this->optional(";")){
      verb = new vproperty_verb(
        *this->file_.get(),
        name.token.line,
        name.token.column,
        name.token.value);
    } else {
      auto body = this->parse_block_statement();
      verb = new vproperty_verb(
        *this->file_.get(),
        name.token.line,
        name.token.column,
        name.token.value,
        body.release());
    }
    
    p->verbs_[name.token.value].reset(verb);
  }
  
  if(this->optional("=")){
    p->init_ = this->parse_expression();
  }
  
  if(!p->result_type_) {
    if(!p->init_){
      throw new compile_error(
        p->owner().file_path(),
        p->line(),
        p->column(),
        "property type is required");
    }
  }
}

std::unique_ptr<vds::vreturn_statement> vds::vfile_syntax::parse_return(const vlex & operator_start)
{
  std::unique_ptr<vds::vreturn_statement> result(
    new vreturn_statement(
      *this->file_.get(),
      operator_start.token.line,
      operator_start.token.column,
      this->parse_expression().release()));
  this->require(";");
  return result;
  
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression0()
{
  if (LT_NAME == this->next_lex_.type) {
    std::unique_ptr<vname_expression> result(
      new vname_expression(
        *this->file_.get(),
        this->next_lex_.token.line,
        this->next_lex_.token.column,
        this->next_lex_.token.value));
    this->next();
    return std::unique_ptr<vds::vexpression>(result.release());
  }

  if (LT_NUMBER == this->next_lex_.type) {
    std::unique_ptr<vnumber_expression> result(
      new vnumber_expression(
        *this->file_.get(),
        this->next_lex_.token.line,
        this->next_lex_.token.column,
        this->next_lex_.token.value));
    this->next();
    return std::unique_ptr<vds::vexpression>(result.release());
  }
  
  if (LT_STRING == this->next_lex_.type) {
    std::unique_ptr<vstring_expression> result(
      new vstring_expression(
        *this->file_.get(),
        this->next_lex_.token.line,
        this->next_lex_.token.column,
        this->next_lex_.token.value));
    this->next();
    return std::unique_ptr<vds::vexpression>(result.release());
  }

  this->unexpected();
  return std::unique_ptr<vds::vexpression>();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression1()
{
  return this->parse_expression0();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression2()
{
  auto result = this->parse_expression1();

  for (;;) {
    if (this->optional(".")) {
      auto name = this->parse_name(false);
      if (!this->optional("(")) {
        result.reset(
          new vproperty_expression(
            result.release(),
            *this->file_.get(),
            name.token.line,
            name.token.column,
            name.token.value
        ));
      }
      else {
        std::unique_ptr<vinvoke_expression> exp(
          new vinvoke_expression(
            result.release(),
            *this->file_.get(),
            name.token.line,
            name.token.column,
            name.token.value));
        this->parse_invoke_arguments(exp->arguments_);
        result.reset(exp.release());
      }

      continue;
    }

    break;
  }

  return result;
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression3()
{
  if (this->optional("new", LT_KEYWORD)) {
    auto type = this->parse_type();
    this->require("(");
    std::unique_ptr<vnew_object_expression> exp(
      new vnew_object_expression(type.release()));
    this->parse_invoke_arguments(exp->arguments_);
    return std::unique_ptr<vds::vexpression>(exp.release());
  }

  return this->parse_expression2();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression4()
{
  return this->parse_expression3();
}


std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression5()
{
  return this->parse_expression4();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression6()
{
  auto result = this->parse_expression5();
  for (;;) {
    vlex operator_start;
    if (this->optional("+", LT_OPERATOR, &operator_start)) {
      auto right = this->parse_expression5();
      result.reset(new vbinary_exptession(
        *this->file_.get(),
        operator_start.token.line,
        operator_start.token.column,        
        "+",
        result.release(),
        right.release()
      ));

      continue;
    }

    if (this->optional("-", LT_OPERATOR, &operator_start)) {
      auto right = this->parse_expression5();
      result.reset(new vbinary_exptession(
        *this->file_.get(),
        operator_start.token.line,
        operator_start.token.column,        
        "-",
        result.release(),
        right.release()
      ));

      continue;
    }

    break;
  }

  return result;
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression7()
{
  return this->parse_expression6();
}


std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression8()
{
  return this->parse_expression7();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression9()
{
  auto result = this->parse_expression8();
  for (;;) {
    vlex operator_start;
    if (this->optional("==", LT_OPERATOR, &operator_start)) {
      auto right = this->parse_expression8();
      result.reset(new vbinary_exptession(
        *this->file_.get(),
        operator_start.token.line,
        operator_start.token.column,        
        "==",
        result.release(),
        right.release()
      ));

      continue;
    }

    break;
  }

  return result;
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression10()
{
  return this->parse_expression9();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression11()
{
  return this->parse_expression10();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression12()
{
  return this->parse_expression11();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression13()
{
  return this->parse_expression12();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression14()
{
  return this->parse_expression13();
}


std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression15()
{
  return this->parse_expression14();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression16()
{
  return this->parse_expression15();
}

std::unique_ptr<vds::vexpression> vds::vfile_syntax::parse_expression()
{
  vds::vlex catched;
  if(this->optional("function", LT_KEYWORD, &catched)){
    std::unique_ptr<vlambda_expression> result(
      new vlambda_expression(
        *this->file_.get(),
        catched.token.line,
        catched.token.column));
    this->parse_lambda(result);
    return std::unique_ptr<vds::vexpression>(result.release());
  }
  
  return this->parse_expression16();
}

vds::vname_expression::vname_expression(
  const vfile & owner,
  int line,
  int column,
  const std::string & name)
: vexpression(owner, line, column), name_(name)
{
}

vds::vproperty_expression::vproperty_expression(
  vexpression * object,
  const vfile & owner,
  int line,
  int column,
  const std::string & name)
: vexpression(owner, line, column), object_(object), name_(name)
{
}

vds::vinvoke_expression::vinvoke_expression(
  vexpression * object,
  const vfile & owner,
  int line,
  int column,
  const std::string & name
) : vexpression(owner, line, column), object_(object), name_(name)
{
}

void vds::vfile_syntax::parse_invoke_arguments(
  std::list<std::unique_ptr<vinvoke_argument>> & arguments)
{
  if(!this->optional(")")){
    for(;;){
      arguments.push_back(this->parse_invoke_argument());
      if(this->optional(",")){
        continue;
      }
      
      this->require(")");
      break;
    }
  }
}

std::unique_ptr<vds::vinvoke_argument> vds::vfile_syntax::parse_invoke_argument()
{
  auto name = this->parse_name(false);
  this->require(":");
  return std::unique_ptr<vds::vinvoke_argument>(
    new vinvoke_argument(
      *this->file_.get(),
      name.token.line,
      name.token.column,
      name.token.value,
      this->parse_expression().release()
  ));
}

std::unique_ptr<vds::vif_statement> vds::vfile_syntax::parse_if(const vlex & operator_start)
{
  std::unique_ptr<vds::vif_statement> result(new vds::vif_statement(
      *this->file_.get(),
      operator_start.token.line,
      operator_start.token.column
  ));
  this->require("(");
  result->condition_ = this->parse_expression();
  this->require(")");
  result->body_ = this->parse_block_statement();
  if (this->optional("else", LT_KEYWORD)) {
    result->else_body_ = this->parse_block_statement();
  }
  
  return result;
}

vds::vnew_object_expression::vnew_object_expression(
  vtype * type)
: vexpression(type->owner(), type->line(), type->column()), type_(type)
{
}

vds::vinvoke_argument::vinvoke_argument(
  const vfile & owner,
  int line,
  int column,
  const std::string & name,
  vds::vexpression * value)
: vlexer_object(owner, line, column), name_(name), value_(value)
{
}

vds::vreturn_statement::vreturn_statement(
  const vfile & owner,
  int line,
  int column,
  vexpression * body)
: vstatement(owner, line, column), body_(body)
{
}

vds::vbinary_exptession::vbinary_exptession(
  const vfile & owner,
  int line,
  int column,
  const std::string & op,
  vexpression * left,
  vexpression * right)
  : vexpression(owner, line, column), op_(op), left_(left), right_(right)
{
}

vds::vnumber_expression::vnumber_expression(
  const vfile & owner,
  int line,
  int column,
  const std::string & value)
  : vexpression(owner, line, column), value_(value)
{
}

vds::vlambda_expression::vlambda_expression(
  const vfile & owner,
  int line,
  int column)
: vexpression(owner, line, column)
{

}

vds::vstring_expression::vstring_expression(const vds::vfile& owner, int line, int column, const std::string& value)
: vexpression(owner, line, column), value_(value)
{
}

vds::vproperty_verb::vproperty_verb(const vds::vfile& owner, int line, int column, const std::string& name)
: vlexer_object(owner, line, column),
name_(name)
{
}

vds::vproperty_verb::vproperty_verb(const vds::vfile& owner, int line, int column, const std::string& name, vds::vblock_statement* block)
: vlexer_object(owner, line, column),
name_(name), block_(block)
{

}

vds::vbase_class::vbase_class(const vds::vfile& owner, int line, int column, const std::string& name)
: vlexer_object(owner, line, column),
name_(name)
{

}
