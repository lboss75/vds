#include "stdafx.h"
#include "vpackage_compiler.h"
#include "vfile_syntax.h"
#include "compile_error.h"
#include "vmethod_compiler.h"
#include "name_resolver.h"
#include "vruntime_machine.h"
#include "vrt_variable.h"
#include "vrt_external_method_resolver.h"
#include "vtype_resolver.h"
#include "vrt_resolve_dependency.h"

vds::vrt_package * vds::vpackage_compiler::compile(
  vds::vruntime_machine * machine,
  const vds::vsyntax & source,
  const vrt_external_method_resolver & external_method_resolver
)
{
  return vpackage_compiler(machine, source).compile(source, external_method_resolver);
}

vds::vpackage_compiler::vpackage_compiler(
  vruntime_machine * machine,
  const vsyntax & source)
  : machine_(machine),
  package_(nullptr), package_file_(nullptr)
{
  this->load_package(source);
  this->load_dependencies();
}

vds::vrt_package * vds::vpackage_compiler::compile(
  const vsyntax & source,
  const vrt_external_method_resolver & external_method_resolver)
{
  auto result = this->machine_->create_package(this->package_->name.token.value);
  std::unique_ptr<vrt_source_file> rt_file(new vrt_source_file(this->package_file_->file_path()));
  result->properties_.reset(static_cast<vrt_json_object *>(this->compile_json(rt_file.get(), this->package_->properties.get())));
  result->source_files_.push_back(std::move(rt_file));
  
  std::list<std::unique_ptr<compiled_item>> compiled_items;

  //collect types
  for(const std::unique_ptr<vfile> & file : source.files()) {
    auto source_file = new vrt_source_file(file->file_path());
    result->source_files_.push_back(std::unique_ptr<vrt_source_file>(source_file));
    for(const std::unique_ptr<vnamespace> & ns : file->namespaces()) {

      std::shared_ptr<vtype_resolver> type_resolver(new vtype_resolver(this->machine_, source_file, this->name_resolver_));
      type_resolver->namespaces_.push_back(ns->name());
      for (auto us : file->usings()) {
        type_resolver->namespaces_.push_back(us.token.value);
      }

      for(const std::unique_ptr<vclass> & cl : ns->classes()) {
        std::unique_ptr<vrt_class> rt_class(new vrt_class(
          result,
          ns->name() + "." + cl->name(),
          source_file,
          cl->line(),
          cl->column()));
        for(const std::unique_ptr<vproperty> & prop : cl->properties()) {
          std::unique_ptr<vrt_property> rt_prop(
            new vrt_property(
              rt_class.get(),
              prop->name()));
          compiled_items.push_back(
            std::unique_ptr<compiled_item>(
              new compiled_property(type_resolver, rt_prop.get(), prop.get())
          ));
          
          rt_class->properties_.push_back(std::move(rt_prop));
        }
        for(const std::unique_ptr<vmethod> & method : cl->methods()) {
          if(method->is_external()) {
            std::unique_ptr<vrt_external_method> rt_method(
              new vrt_external_method(
              source_file,
              method->line(), method->column(),
              rt_class.get(),               
              method->name()));

            compiled_items.push_back(
              std::unique_ptr<compiled_item>(
                new compiled_external_method(
                  type_resolver,
                  rt_method.get(),
                  method.get(),
                  external_method_resolver)
                ));

            rt_class->methods_.push_back(std::move(rt_method));
          } else {
            std::unique_ptr<vrt_method> rt_method(
              new vrt_method(
                source_file,
                method->line(), method->column(),
                rt_class.get(),               
                method->name()));

            compiled_items.push_back(
              std::unique_ptr<compiled_item>(
                new compiled_method(
                  type_resolver,
                  rt_method.get(),
                  method.get())
                ));

            rt_class->methods_.push_back(std::move(rt_method));
          }
        }
        
        compiled_items.push_back(
          std::unique_ptr<compiled_item>(
            new compiled_class(type_resolver, rt_class.get(), cl.get())
        ));
        result->classes_.push_back(std::move(rt_class));
      }        
    }
  }
  
  this->name_resolver_.add(result);
  
  for (auto & item : compiled_items) {
    item->resolve_types();
  }

  for (auto & item : compiled_items) {
    item->compile();
  }

  return result;
}

void vds::vpackage_compiler::load_package(const vds::vsyntax& source)
{
  for(const std::unique_ptr<vfile> & file : source.files()) {
    if (file->package_) {
      if(nullptr != this->package_) {
        throw new compile_error(
          file->file_path(),
          file->package_->name.token.line,
          file->package_->name.token.column,
          "Package already exists");
      }
      
      this->package_file_ = file.get();
      this->package_ = file->package_.get();
    }
  }
  
  if(nullptr == this->package_) {
    throw new compile_error(
      "",
      -1,
      -1,
      "Package not found");
  }
}

vds::vrt_json_value* vds::vpackage_compiler::compile_json(
  const vds::vrt_source_file* source_file,
  const vds::vjson_value* object)
{
  auto primitive = dynamic_cast<const vjson_primitive *>(object);
  if(nullptr != primitive){
    return new vrt_json_primitive(
      source_file,
      object->line(),
      object->column(),
      primitive->value()
    );
  }
  
  auto obj = dynamic_cast<const vjson_object *>(object);
  if(nullptr != obj){
    std::unique_ptr<vrt_json_object> result(new vrt_json_object(
      source_file,
      object->line(), object->column()
    ));
    obj->visit([source_file, &result](const vjson_property & prop){
      result->properties_.push_back(
        vrt_json_property {
          prop.name,
          std::unique_ptr<vrt_json_value>(compile_json(source_file, prop.value.get()))
        }
      );
    });
    
    return result.release();  
  }

  auto array = dynamic_cast<const vjson_array *>(object);
  if (nullptr != array) {
    std::unique_ptr<vrt_json_array> result(new vrt_json_array(
      source_file,
      object->line(), object->column()
    ));

    for (size_t i = 0; i < array->size(); ++i) {
      result->items_.push_back(std::unique_ptr<vrt_json_value>(compile_json(source_file, array->get(i))));
    }

    return result.release();
  }

  throw new std::runtime_error("Invalid json type");

}

void vds::vpackage_compiler::load_dependencies()
{
  this->package_->properties->visit([this](const vjson_property& property) {
    if ("dependencies" == property.name) {
      auto dependencies = dynamic_cast<const vjson_object *>(property.value.get());
      if (nullptr != dependencies) {
        dependencies->visit([this](const vjson_property & property) {
          auto package = this->machine_->get_package(property.name);
          if (nullptr == package) {
            throw new compile_error(
              this->package_file_->file_path(),
              property.line,
              property.column,
              "Package " + property.name + " not found");
          }
          this->name_resolver_.add(package);
        });
      }
    }
  });
}
////////////////////////////////////////////////////////////////////////////////////
vds::vpackage_compiler::compiled_item::~compiled_item()
{
}
////////////////////////////////////////////////////////////////////////////////////
vds::vpackage_compiler::compiled_property::compiled_property(
  const std::shared_ptr<vtype_resolver> & resolver,
  vrt_property * compiled,
  vproperty * original)
  : type_resolver_(resolver), compiled_(compiled), original_(original)
{
}

void vds::vpackage_compiler::compiled_property::resolve_types()
{
  this->compiled_->property_type_ = this->type_resolver_->resolve_type(this->original_->result_type());
}

void vds::vpackage_compiler::compiled_property::compile()
{
  if (this->original_->is_dependency()) {
    auto method = new vrt_method(
      this->type_resolver_->file_,
      this->original_->line(),
      this->original_->column(),
      this->compiled_->declaring_type(),
      this->compiled_->name() + "$" + vrt_property::verb_get()
    );

    method->body_.push_back(
      std::unique_ptr<vrt_statement>(
        new vrt_resolve_dependency(
          this->type_resolver_->file_,
          this->original_->line(),
          this->original_->column(),
          this->compiled_->property_type_)));

    this->compiled_->methods_[vrt_property::verb_get()] = std::unique_ptr<vrt_callable>(method);
  }
}
////////////////////////////////////////////////////////////////////////////////////
vds::vpackage_compiler::compiled_method::compiled_method(
  const std::shared_ptr<vtype_resolver> & resolver,
  vrt_method * compiled,
  vmethod * original)
  : type_resolver_(resolver), compiled_(compiled), original_(original)
{
}

void vds::vpackage_compiler::compiled_method::resolve_types()
{
  this->type_resolver_->resolve_types(this->compiled_, this->original_);
}

void vds::vpackage_compiler::compiled_method::compile()
{
  if (nullptr == this->original_->body()) {
    if (!this->original_->declaring_type()->is_interface()) {
      throw new std::runtime_error("Method have to contains body");
    }
  }
  else {
    vmethod_compiler compiler(
      this->type_resolver_.get(),
      this->compiled_);
    compiler.compile(this->original_);
  }
}
////////////////////////////////////////////////////////////////////////////////////
vds::vpackage_compiler::compiled_external_method::compiled_external_method(
  const std::shared_ptr<vtype_resolver> & resolver,
  vrt_external_method * compiled,
  vmethod * original,
  const vrt_external_method_resolver & external_method_resolver)
  : external_method_resolver_(external_method_resolver),
  type_resolver_(resolver), compiled_(compiled), original_(original)
{
}

void vds::vpackage_compiler::compiled_external_method::resolve_types()
{
  this->type_resolver_->resolve_types(this->compiled_, this->original_);
}

void vds::vpackage_compiler::compiled_external_method::compile()
{
  this->compiled_->impl_ = this->external_method_resolver_.resolve(this->compiled_);
}
/////////////////////////////////////////////////////////////////////////////////////
vds::vpackage_compiler::compiled_class::compiled_class(
  const std::shared_ptr< vds::vtype_resolver >& resolver,
  vds::vrt_class* compiled,
  vds::vclass* original)
: type_resolver_(resolver), compiled_(compiled), original_(original)
{
}

void vds::vpackage_compiler::compiled_class::resolve_types()
{
}

void vds::vpackage_compiler::compiled_class::compile()
{
  bool have_primitive_constructor = false;
  for(auto & constructor : this->compiled_->constructors()){
    if(constructor->parameters().empty()){
      have_primitive_constructor = true;
      break;
    }
  }
  
  if(!have_primitive_constructor){
    std::unique_ptr<vrt_constructor> rt_constructor(
      new vrt_constructor(
        this->compiled_->file(),
        this->compiled_->line(),
        this->compiled_->column(),
        this->compiled_));
      
    vmethod_compiler compiler(
      this->type_resolver_.get(),
      rt_constructor.get());
    compiler.generate_primitive_constructor(this->original_);
    
    this->compiled_->constructors_.push_back(std::move(rt_constructor));
  }
}


