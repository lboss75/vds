#ifndef __VRT_PACKAGE_H_
#define __VRT_PACKAGE_H_

#include <vector>
#include "vrt_variable_declaration.h"

namespace vds {
  class vrt_class;
  class vrt_context;
  class vrt_json_object;
  class vrt_object;
  class vrt_statement;
  class vrt_package;
  class vrt_variable;
  class vrt_variable_declaration;
  class vrt_type;
  
  class vrt_source_file
  {
  public:
    vrt_source_file(const std::string & file_path);
    
    const std::string & file_path() const
    {
      return this->file_path_;
    }
    
  private:
    std::string file_path_;
  };
  
  class vrt_member_info
  {
  public:
    vrt_member_info(
      const vrt_type * declaring_type,
      const std::string & name);
    
    virtual ~vrt_member_info();
    
    const vrt_type * declaring_type() const {
      return this->declaring_type_;
    }
    
    const std::string & name() const {
      return this->name_;
    }
    
  private:
    const vrt_type * declaring_type_;
    std::string name_;    
  };
  
  class vrt_parameter : public vrt_variable_declaration
  {
  public:
    vrt_parameter(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_type * type,
      const std::string & name
    );
  };
  
  class vrt_callable : public vrt_member_info
  {
  public:
    vrt_callable(
      const vrt_type * declaring_type,
      const std::string & name);

    const vrt_type * get_result_type() const {
      return this->result_type_;
    }

    const std::vector<std::unique_ptr<vrt_parameter>> & parameters() const {
      return this->parameters_;
    }

    size_t get_parameter_index(const std::string & name) const;

  private:
    friend class vtype_resolver;
    const vrt_type * result_type_;
    std::vector<std::unique_ptr<vrt_parameter>> parameters_;
  };
  
  class vrt_external_method : public vrt_callable
  {
  public:
    vrt_external_method(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_type * declaring_type,
      const std::string & name
    );
    
    bool invoke(
      vrt_context & context,
      const std::shared_ptr<vrt_object> & pthis,
      const std::map<std::string, std::shared_ptr<vrt_object>> & arguments
      ) const;
    
  private:
    friend class vpackage_compiler;
    std::function<
      bool (
        vrt_context & context,
        const std::shared_ptr<vrt_object> & pthis,
        const std::map<std::string, std::shared_ptr<vrt_object>> & arguments)> impl_;
  };
  
  class vrt_method : public vrt_callable
  {
  public:
    vrt_method(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_type * declaring_type,
      const std::string & name);
    

    const std::vector<std::unique_ptr<vrt_statement>> & body() const
    {
      return this->body_;
    }
    
  private:
    friend class vmethod_compiler;
    friend class vpackage_compiler;
    friend class vrt_context;
    friend class vrt_method_context;

    const vrt_source_file * file_;
    int line_;
    int column_;
    
    std::vector<std::unique_ptr<vrt_statement>> body_;
    std::vector<std::unique_ptr<vrt_variable_declaration>> variables_;
  };
  
  class vrt_constructor : public vrt_method
  {
  public:
    vrt_constructor(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_type * declaring_type
    );
    
  private:
    friend class vmethod_compiler;
  };
  
  class vrt_property : public vrt_member_info
  {
  public:
    static const std::string & verb_get();

    vrt_property(
      const vrt_type * declaring_type,
      const std::string & name
    );

    const vrt_callable * get_method(const std::string & verb) const {
      auto p = this->methods_.find(verb);
      if(this->methods_.end() == p){
        return nullptr;
      }
      
      return p->second.get();
    }

    const vrt_type * get_property_type() const {
      return this->property_type_;
    }
    
  private:
    friend class vpackage_compiler;
    const vrt_type * property_type_;
    std::map<std::string, std::unique_ptr<vrt_callable>> methods_;
  };
  
  class vrt_type
  {
  public:
    vrt_type(const vrt_package * package, const std::string & name);
    virtual ~vrt_type();
    
    const vrt_package * package() const {
      return this->package_;
    }

    const std::string & name() const
    {
      return this->name_;
    }

    std::string full_name() const;

  private:
    const vrt_package * package_;
    std::string name_;
  };
  
  class vrt_class : public vrt_type
  {
  public:
    vrt_class(
      const vrt_package * package,
      const std::string & name,
      const vrt_source_file * file,
      int line,
      int column);
    
    const std::list<std::unique_ptr<vrt_constructor>> & constructors() const {
      return this->constructors_;
    }
    
    const std::list<std::unique_ptr<vrt_callable>> & methods() const {
      return this->methods_;
    }
    
    const std::list<std::unique_ptr<vrt_property>> & properties() const {
      return this->properties_;
    }
    
    const vrt_source_file * file() const {
      return this->file_;
    }
    
    int line() const {
      return this->line_;
    }
    
    int column() const {
      return this->column_;
    }
    
  private:
    const vrt_source_file * file_;
    int line_;
    int column_;
    std::list<std::unique_ptr<vrt_constructor>> constructors_;
    std::list<std::unique_ptr<vrt_callable>> methods_;
    std::list<std::unique_ptr<vrt_property>> properties_;
    
    friend class vpackage_compiler;
  };
  
  class vrt_generic
  {
  public:
    
  };
  
  class vrt_generic_class : public vrt_class
  {
  public:
    
  private:
    const vrt_generic * generic_;
    std::vector<const vrt_type *> arguments_;
  };
  

  class vrt_package
  {
  public:
    vrt_package(const std::string & name);

    const std::string & name() const {
      return this->name_;
    }

    const std::list<std::unique_ptr<vrt_class>> & classes() const
    {
      return this->classes_;
    }
    
    const vrt_class * get_class(const std::string & name, bool throwError = true) const;

    const vrt_json_object * properties() const {
      return this->properties_.get();
    }

  private:
    std::list<std::unique_ptr<vrt_class>> classes_;
    std::list<std::unique_ptr<vrt_source_file>> source_files_;
    std::string name_;

    std::unique_ptr<vrt_json_object> properties_;
    
    friend class vpackage_compiler;
  };

}

#endif // __VRT_PACKAGE_H_
