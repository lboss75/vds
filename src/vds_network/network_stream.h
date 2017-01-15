/* inclusion guard */
#ifndef __VDS_NETWORK_NETWORK_STREAM_H__
#define __VDS_NETWORK_NETWORK_STREAM_H__

namespace vds {
  class network_stream
  {
  public:
    
    template <
      typename next_method_type,
      typename error_method_type
      >
    class source
    {
    public:
      source(
        next_method_type next,
        error_method_type on_error,
        const network_stream & args)
      : task_(next, on_error)
      {
      }
      
      void operator()() {
        this->task_.schedule(network_manager_);        
      }
      
    private:
      network_manager * network_manager_;
      read_socket_task<next_method_type, error_method_type> task_;
    };
    
    template <
      typename done_method_type,
      typename error_method_type
      >
    class target
    {
    public:
      source(
        done_method_type done,
        error_method_type on_error,
        const network_stream & args)
      : task_(done, on_error)
      {
      }
      
      void operator()(
        const void * data,
        size_t len
      ) {
        this->task_.set_data(data, len);
        this->task_.schedule(network_manager_);        
      }
      
    private:
      network_manager * network_manager_;
      write_socket_task<done_method_type, error_method_type> task_;
    };
  };
}

#endif /* __VDS_NETWORK_NETWORK_STREAM_H__ */
