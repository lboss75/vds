#include "stdafx.h"
#include "vmethod_compiler.h"
#include "name_resolver.h"
#include "vrt_package.h"
#include "vfile_syntax.h"
#include "compile_error.h"
#include "vrt_expression.h"
#include "vtype_resolver.h"

vds::vmethod_compiler::vmethod_compiler(
  vtype_resolver * resolver,
  vrt_method * target)
: resolver_(resolver),
target_(target)
{
}

void vds::vmethod_compiler::compile(vmethod * source)
{
  if(!source->is_static()){
    this->register_variable(
      source->line(), source->column(),
      this->target_->declaring_type(),
      "this"
    );
  }
  
  for(const std::unique_ptr<vrt_parameter> & p : this->target_->parameters()){
    this->register_variable(
      source->line(), source->column(),
      p->type(),
      p->name()
    );
  }
  
  this->compile_block(source->body());
}



void vds::vmethod_compiler::compile_block(
  const vblock_statement * statement)
{
  for(const std::unique_ptr<vstatement> & step : statement->statements()) {
    this->compile(step.get());
  }
}

void vds::vmethod_compiler::compile(const vds::vstatement* statement)
{
  auto exp = dynamic_cast<const vexpression_statement *>(statement);
  if(nullptr != exp) {
    this->compile_expression(exp->expression());
    this->push(
        new vrt_pop_statement(
          this->resolver_->file_,
          statement->line(),
          statement->column()
        ));
    return;
  }
  
  auto var_st = dynamic_cast<const var_statement *>(statement);
  if(nullptr != var_st) {
    this->compile_var_statement(var_st);
    return;
  }

  auto ret_st = dynamic_cast<const vreturn_statement *>(statement);
  if (nullptr != ret_st) {
    this->compile_return_statement(ret_st);
    return;
  }
  
  throw new compile_error(
    statement->owner().file_path(),
    statement->line(),
    statement->column(),
    "Unexpected statement"
  );
}

void vds::vmethod_compiler::compile_var_statement(const var_statement* st)
{
  const vrt_type * type = nullptr;
  
  if(st->type()){
    type = this->resolver_->resolve_type(st->type());
  }
  
  const vexpression * init_exp = st->init_value();
  if(nullptr != init_exp){
    auto init_type = this->compile_expression(init_exp);
    if (nullptr == type) {
      type = init_type;
    }
  }
  
  if(nullptr == type) {
    throw new compile_error(
      st->owner().file_path(),
      st->line(),
      st->column(),
      "Variable type is required"
    );
  }
  
  auto index = this->register_variable(
    st->line(), st->column(),
    type,
    st->name());
  
  if(nullptr != init_exp){
    this->push(
      new vrt_assign_var_statement(
        this->resolver_->file_,
        st->line(),
        st->column(),
        index
      ));
  }
}

void vds::vmethod_compiler::compile_return_statement(const vreturn_statement * st)
{
  auto type = this->compile_expression(st->body());
  this->push(
    new vrt_return_statement(
      this->resolver_->file_,
      st->line(),
      st->column(),
      type
    ));

}

const vds::vrt_type * vds::vmethod_compiler::compile_expression(
  const vds::vexpression* ex)
{
  auto create_obj = dynamic_cast<const vnew_object_expression *>(ex);
  if(nullptr != create_obj) {
    return this->compile_new_object_expression(create_obj);
  }
  
  auto str_exp = dynamic_cast<const vstring_expression *>(ex);
  if (nullptr != str_exp) {
    this->push(
    new vrt_string_const(
      this->resolver_->file_,
      str_exp->line(),
      str_exp->column(),
      str_exp->value()));
    return this->resolver_->resolve_type("string,core");
  }
  
  auto num_exp = dynamic_cast<const vnumber_expression *>(ex);
  if (nullptr != num_exp) {
    this->push(
      new vrt_number_const(
        this->resolver_->file_,
        num_exp->line(),
        num_exp->column(),
        num_exp->value()));
    return this->resolver_->resolve_type("number,core");
  }
  
  auto prop_exp = dynamic_cast<const vproperty_expression *>(ex);
  if (nullptr != prop_exp) {
    auto result_type = this->compile_property(prop_exp);
    if (nullptr != result_type) {
      return result_type;
    }

    auto type = this->compile_expression(prop_exp->object());
    auto prop = this->resolve_property(type, prop_exp->name());
    if (nullptr == prop) {
      throw new compile_error(
        ex->owner().file_path(),
        ex->line(),
        ex->column(),
        "Property " + prop_exp->name() + " not found"
      );
    }
    this->push(
      new vrt_get_property(
        this->resolver_->file_,
        prop_exp->line(),
        prop_exp->column(),
        prop));
    return prop->get_property_type();
  }

  auto invoke_exp = dynamic_cast<const vinvoke_expression *>(ex);
  if (nullptr != invoke_exp) {
    auto object_type = this->compile_expression(invoke_exp->object());

    std::list<std::string> arguments;
    std::map<std::string, const vrt_type *> argument_types;
    for (const std::unique_ptr<vinvoke_argument> & argument : invoke_exp->arguments()) {
      arguments.push_back(argument->name());
      argument_types[argument->name()] = this->compile_expression(argument->value());
    }

    auto method = this->resolver_->machine_->resolve_method(
      object_type, invoke_exp->name(), argument_types);
    if (nullptr == method) {
      throw new compile_error(
        ex->owner().file_path(),
        ex->line(),
        ex->column(),
        "Method " + invoke_exp->name() + " not found"
      );
    }
    this->push(
      new vrt_method_invoke(
        this->resolver_->file_,
        ex->line(),
        ex->column(),
        method,
        arguments));
    return method->get_result_type();
  }

  auto name_exp = dynamic_cast<const vname_expression *>(ex);
  if (nullptr != name_exp) {
    return this->compile_name(name_exp);
  }

  throw new compile_error(
    ex->owner().file_path(),
    ex->line(),
    ex->column(),
    "Unexpected expression of type " + std::string(typeid(ex).name())
  );
}

const vds::vrt_type * vds::vmethod_compiler::compile_property(const vds::vproperty_expression * ex)
{
  std::list<std::pair<std::string, const vexpression *>> names;
  names.push_back(std::pair<std::string, const vexpression *>(ex->name(), ex));
  const vds::vexpression * parent = ex->object();
  for (;;) {
    auto p = dynamic_cast<const vproperty_expression *>(parent);
    if (nullptr != p) {
      names.push_back(std::pair<std::string, const vexpression *>(p->name(), p));
      parent = p->object();
      continue;
    }

    break;
  }

  auto top_name = dynamic_cast<const vname_expression *>(parent);
  if (nullptr == top_name) {
    return nullptr;
  }

  names.push_back(std::pair<std::string, const vexpression *>(top_name->name(), top_name));
  
  for(std::list<std::pair<std::string, const vexpression *>>::const_iterator p = names.begin(); p != names.end(); ++p) {
    std::string name;
    for(std::list<std::pair<std::string, const vexpression *>>::const_iterator p1 = p; p1 != names.end(); ++p1) {
      if(name.empty()){
        name = p1->first;
      } else {
        name = p1->first + "." + name;
      }
    }    
    
    auto pvariable = this->variables_.find(name);
    const vrt_type * type = nullptr;
    if(this->variables_.end() != pvariable) {
      this->push(
        new vrt_localvariable_reference(
          this->resolver_->file_,
          top_name->line(),
          top_name->column(),
          pvariable->second.index));
      type = pvariable->second.type;
    } else {
      type = this->resolver_->resolve_name(*this, name);
      if(nullptr == type) {
        type = this->resolver_->resolve_type(name, false);
        if(nullptr == type) {
          continue;
        }
      }
    }
    
    if(p != names.begin()){
      do{
        --p;
        auto prop = this->resolve_property(type, p->first);
        if(nullptr == prop){
          throw new compile_error(
            p->second->owner().file_path(),
            p->second->line(),
            p->second->column(),
            "Property " + p->first + " not found in type " + type->name()
          );
        }
        this->push(
          new vrt_get_property(
            this->resolver_->file_,
            p->second->line(),
            p->second->column(),
            prop));
        type = prop->get_property_type();
      } while(p != names.begin());
    }
    
    return type;
  }
  
  return nullptr;
}

const vds::vrt_type * vds::vmethod_compiler::compile_name(const vds::vname_expression * ex)
{
  auto pvariable = this->variables_.find(ex->name());
  if (this->variables_.end() != pvariable) {
    this->push(
      new vrt_localvariable_reference(
        this->resolver_->file_,
        ex->line(),
        ex->column(),
        pvariable->second.index));
    return pvariable->second.type;
  }
  else {
    auto type = this->resolver_->resolve_name(*this, ex->name());
    if (nullptr == type) {
      type = this->resolver_->resolve_type(ex->name());
      if (nullptr == type) {
        throw new compile_error(
          ex->owner().file_path(),
          ex->line(),
          ex->column(),
          "Name " + ex->name() + " not found"
        );
      }
    }

    return type;
  }
}

size_t vds::vmethod_compiler::register_variable(
  int line, int column, const vrt_type * type, const std::string & name)
{
  if(this->variables_.end() != this->variables_.find(name)){
    throw new compile_error(
      this->resolver_->file_->file_path(),
      line,
      column,
      "Variable " + name + " already defined");
  }
  
  variable_info vi;
  vi.index = this->variables_.size();
  vi.type = type;
  this->variables_[name] = vi;
  return vi.index;
}

const vds::vrt_type * vds::vmethod_compiler::compile_new_object_expression(const vds::vnew_object_expression* ex)
{
  auto type = this->resolver_->resolve_type(ex->type());
  std::list<std::string> arguments;
  std::map<std::string, const vrt_type *> argument_types;
  for (const std::unique_ptr<vinvoke_argument> & argument : ex->arguments()) {
    if (argument_types.end() != argument_types.find(argument->name())) {
      throw new compile_error(
        argument->owner().file_path(),
        argument->line(),
        argument->column(),
        "Argument " + argument->name() + " already used");
    }
    
    auto type = this->compile_expression(argument->value());

    arguments.push_front(argument->name());
    argument_types[argument->name()] = type;
  }
  
  auto constructor = this->resolver_->machine_->resolve_constructor(type, argument_types);

  this->push(
    new vrt_new_object_expression(
      this->resolver_->file_,
      ex->line(),
      ex->column(),
      constructor,
      arguments));
  return type;
}


const vds::vrt_property* vds::vmethod_compiler::resolve_property(
  const vds::vrt_type* type,
  const std::string& name)
{
  auto cls = dynamic_cast<const vrt_class *>(type);
  if (nullptr != cls) {
    
    for (const std::unique_ptr<vrt_property> & prop : cls->properties()) {
      if (prop->name() == name) {
        return prop.get();
      }
    }
  }
  
  return nullptr;
}

void vds::vmethod_compiler::push(vds::vrt_statement* st) const
{
  static_cast<vrt_method *>(this->target_)->body_.push_back(
    std::unique_ptr<vrt_statement>(
      st));
}

void vds::vmethod_compiler::generate_primitive_constructor(vclass* cls)
{
  this->push(
    new vrt_return_statement(
      this->target_->file_,
      this->target_->line_,
      this->target_->column_,
      nullptr
    ));
}
