#ifndef __NAME_RESOLVER_H_
#define __NAME_RESOLVER_H_

namespace vds {
  class vmethod_compiler;
  class vrt_package;
  class vrt_type;
  
  class name_resolver
  {
  public:
    void add(const vrt_package * package);
    void add_name(
      const std::string & name,
      const std::function<const vrt_type * (vmethod_compiler &)>& factory);
    const vrt_type * resolve_type(const std::string & type_name);
    
    const vrt_type * resolve_name(vmethod_compiler & compiler, const std::string & name);
    
  private:
    std::list<const vrt_package *> packages_;
    std::map<std::string, const vrt_class *> classes_;
    std::map<std::string, std::function<const vrt_type * (vmethod_compiler &)>> names_;
  };
}

#endif // !__NAME_RESOLVER_H_
