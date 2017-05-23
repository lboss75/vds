#ifndef __VDS_HTTP_HTTP_MIDDLEWARE_H_
#define __VDS_HTTP_HTTP_MIDDLEWARE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"
#include "http_router.h"

namespace vds {
  class http_router;
  
  template <typename router_type>
  class http_middleware
  {
  public:
    http_middleware(const router_type & router)
    : router_(router){
    }    

    using incoming_item_type = std::shared_ptr<http_message>;
    using outgoing_item_type = std::shared_ptr<http_message>;
    static constexpr size_t BUFFER_SIZE = 10;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public vds::sync_dataflow_filter<context_type, handler<context_type>>
    {
      using base_class = vds::sync_dataflow_filter<context_type, handler<context_type>>;

    public:
      handler(
        const context_type & context,
        const http_middleware & params)
      : base_class(context),
        router_(params.router_)
      {
      }
      
      void sync_process_data(const vds::service_provider & sp, size_t & input_readed, size_t & output_written)
      {
        auto n = (this->input_buffer_size_ < this->output_buffer_size_)
          ? this->input_buffer_size_
          : this->output_buffer_size_;

        for (size_t i = 0; i < n; ++i) {
          try {
            this->output_buffer_[i] = this->router_.route(sp, this->input_buffer_[i]);
          }
          catch(...) {
            std::list<std::string> headers;
            //std::string body = exception_what(std:: )
            //auto result = std::make_shared<http_message>();
            //this->output_buffer_[i] = this->http_error_handler_(sp, std::current_exception());
          }
        }

        input_readed = n;
        output_written = n;
      }
      
    private:
      const router_type & router_;
    };
    
  private:
    const router_type & router_;
  };
}

#endif // __VDS_HTTP_HTTP_MIDDLEWARE_H_
