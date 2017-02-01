#ifndef __VTYPE_RESOLVER_H_
#define __VTYPE_RESOLVER_H_

namespace vds {
  class name_resolver;
  class vmethod;
  class vmethod_compiler;
  class vrt_source_file;
  class vruntime_machine;
  class vtype;
  
  class vtype_resolver
  {
  public:
    vtype_resolver(
      vruntime_machine * machine,
      const vrt_source_file * file,
      name_resolver & resolver);
    
    const vrt_type * resolve_type(
      const vtype * t, bool throw_error = true);
    
    const vrt_type * resolve_name(vmethod_compiler & compiler, const std::string & name);
    
    void resolve_types(vrt_callable * target, const vmethod * source);
    
  protected:
    friend class vpackage_compiler;
    friend class vmethod_compiler;
    
    vruntime_machine * machine_;
    const vrt_source_file * file_;
    std::list<std::string> namespaces_;
    name_resolver & resolver_;    
    
    const vrt_type * resolve_type(const std::string & name, bool throw_error = true);
  };
}

#endif // __VTYPE_RESOLVER_H_
