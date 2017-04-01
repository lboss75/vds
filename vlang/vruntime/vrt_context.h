#ifndef __VRT_CONTEXT_H_
#define __VRT_CONTEXT_H_

namespace vds {
  class vrt_callable;
  class vrt_class;
  class vrt_constructor;
  class vrt_method_context;
  class vrt_object;
  class vrt_try_context;
  class vrt_type;
  class vrt_variable;
  class vruntime_machine;
  
  class vrt_context : public std::enable_shared_from_this<vrt_context>
  {
  public:
    vrt_context(const vrt_context &) = delete;
    vrt_context(vrt_context &&) = delete;

    vrt_context(
      vruntime_machine * machine,
      const std::function<void(vrt_context &)> & done,
      const error_handler_t & on_error
      );
    
    bool invoke(
      const std::shared_ptr<vrt_object> & pthis,
      const vrt_callable * method,
      const std::map<std::string, std::shared_ptr<vrt_object>> & arguments
      = std::map<std::string, std::shared_ptr<vrt_object>>()
      );
    
    std::shared_ptr<vrt_object> pop();
    void push(const std::shared_ptr<vrt_object> & value);
    
    void method_return(
      const std::shared_ptr<vrt_object> & value);
    
    void method_return();
    
    void push_string_const(
      const std::string & value);

    void push_number_const(
      const std::string & value);

   
    vrt_method_context * get_current_method() const;

    const simple_done_handler_t & done_handler() const
    {
      return this->done_handler_;
    }

    const error_handler_t & error_handler() const
    {
      return this->error_handler_;
    }

    bool resolve_dependency(const vrt_type * interface_type);

  private:
    friend class vruntime_machine;
    
    vruntime_machine * machine_;
    std::stack<std::unique_ptr<vrt_method_context>> methods_;
    simple_done_handler_t done_handler_;
    error_handler_t error_handler_;
    std::function<void(vrt_context &)> final_done_;
    error_handler_t final_error_;

    void execute_continue();
    bool throw_error(std::exception_ptr);

    bool execute_step();

  };
}

#endif // __VRT_CONTEXT_H_
