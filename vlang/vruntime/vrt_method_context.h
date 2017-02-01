#ifndef __VRT_METHOD_CONTEXT_H_
#define __VRT_METHOD_CONTEXT_H_

#include <stack>

namespace vds {
  class vrt_context;
  class vrt_method;
  class vrt_object;
  class vrt_variable;
  class vrt_scope;

  class vrt_method_context
  {
  public:
    vrt_method_context(
      vrt_context & context,
      const std::shared_ptr< vds::vrt_object >& pthis,
      const vrt_method * method,
      const std::map<std::string, std::shared_ptr<vrt_object>> & arguments);
    
    std::shared_ptr<vrt_object> pop();
    void push(const std::shared_ptr<vrt_object> & value);
    
    const std::shared_ptr<vrt_variable> & get_variable(size_t index) const {
      return this->variables_[index];
    }
  private:
    friend class vrt_context;

    vrt_context & context_;
    std::shared_ptr<vrt_object> this_;
    const vrt_method * method_;
    size_t next_statement_;
    std::stack<std::shared_ptr<vrt_object>> stack_;
    std::vector<std::shared_ptr<vrt_variable>> variables_;
    std::stack<std::unique_ptr<vrt_scope>> scopes_;
    
    //std::stack<std::unique_ptr<vrt_try_context>> try_contexts_;


    bool execute_step(vrt_context & context);
  };
};

#endif // __VRT_METHOD_CONTEXT_H_
