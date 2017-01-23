#ifndef __VDS_CORE_PIPELINE_FILTER_H_
#define __VDS_CORE_PIPELINE_FILTER_H_

namespace vds {
  ////////////////////////////////////////////////////////////////
  template <typename method_type, typename method_signature>
  class _method_proxy;

  template <typename method_type, typename... arg_types>
  class _method_proxy<method_type, void (arg_types...)>
  {
  public:
    _method_proxy(method_type & method)
      : method_(method)
    {
    }
    
    void operator()(arg_types&&... args)
    {
      this->method_(std::move(args)...);
    }

  private:
    method_type & method_;
  };
  ////////////////////////////////////////////////////////////////
  template <typename method_type, typename method_signature>
  class _processed_method_proxy;

  template <typename method_type, typename... arg_types>
  class _processed_method_proxy<method_type, void(arg_types...)>
  {
  public:
    _processed_method_proxy(method_type & method)
      : method_(method)
    {
    }

    void operator()(arg_types&&... args)
    {
      this->method_.processed(std::move(args)...);
    }

  private:
    method_type & method_;
  };
  ////////////////////////////////////////////////////////////////
  template <
    typename prev_filter_type,
    typename next_filter_type,
    typename error_method_type
  >
  class pipeline_context
  {
  public:
    typedef prev_filter_type prev_filter_t;
    typedef next_filter_type next_filter_t;
    typedef error_method_type error_method_t;

    pipeline_context(
      prev_filter_t & prev,
      next_filter_t & next,
      error_method_t & error
    ) : prev_(prev), next_(next), error_(error)
    {
    }

    prev_filter_t & prev_;
    next_filter_t & next_;
    error_method_t & error_;
  };

  ////////////////////////////////////////////////////////////////
  template<
    typename context_type,
    typename output_signature,
    typename processed_signature = void(void)
  >
  class pipeline_filter
  {
  public:
    pipeline_filter(
      const context_type & context
    )
    : processed(context.prev_),
      next(context.next_),
      error(context.error_)
    {
    }

    _processed_method_proxy<context_type::prev_filter_t, processed_signature> processed;
    _method_proxy<context_type::next_filter_t, output_signature> next;
    _method_proxy<context_type::error_method_t, void(std::exception *)> error;
  };
}

#endif//__VDS_CORE_PIPELINE_FILTER_H_