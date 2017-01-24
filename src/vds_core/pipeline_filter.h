#ifndef __VDS_CORE_PIPELINE_FILTER_H_
#define __VDS_CORE_PIPELINE_FILTER_H_

namespace vds {
  ////////////////////////////////////////////////////////////////
  template <typename method_type, typename method_signature>
  class _pipeline_method_proxy;

  template <typename method_type, typename... arg_types>
  class _pipeline_method_proxy<method_type, void(arg_types...)>
  {
  public:
    _pipeline_method_proxy(method_type & method)
      : method_(method)
    {
    }

    template <typename done_mehtod_type>
    void operator()(done_mehtod_type & done, arg_types... args)
    {
      this->method_(done, args...);
    }

  private:
    method_type & method_;
  };

  ////////////////////////////////////////////////////////////////
  template<
    typename context_type,
    typename output_signature
  >
  class pipeline_filter
  {
  public:
    typedef pipeline_filter base;

    pipeline_filter(
      const context_type & context
    )
      : next(context.next_),
      error(context.error_)
    {
    }
#if _DEBUG
    typedef _pipeline_method_proxy<typename context_type::next_step_t, output_signature> next_step_t;
    typedef _method_proxy<typename context_type::error_method_t, void(std::exception *)> error_method_t;
#else
    typename context_type::next_step_t & next_step_t;
    typename context_type::error_method_t & error_method_t;
#endif

    next_step_t next;
    error_method_t error;
  };
}

#endif//__VDS_CORE_PIPELINE_FILTER_H_