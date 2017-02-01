#ifndef __VRT_EXTERNAL_METHOD_RESOLVER_H_
#define __VRT_EXTERNAL_METHOD_RESOLVER_H_

namespace vds {
  class vrt_external_method;
  
  class vrt_external_method_resolver
  {
  public:
    typedef std::function<
      bool (
        vrt_context & context,
        const std::shared_ptr<vrt_object> & pthis,
        const std::map<std::string, std::shared_ptr<vrt_object>> & arguments)>
    method_body_t;
    
    method_body_t resolve(const vrt_external_method * method) const;
    
    void add(const std::string & name, const method_body_t method);
  private:
    std::map<std::string, method_body_t> methods_;
  };
}

#endif // __VRT_EXTERNAL_METHOD_RESOLVER_H_
