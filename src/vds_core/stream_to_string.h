#ifndef __VDS_CORE_STREAM_TO_STRING_H_
#define __VDS_CORE_STREAM_TO_STRING_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  
  class stream_to_string
  {
  public:
    stream_to_string()
    {
    }
    
    template <
      typename done_method_type,
      typename next_method_type,
      typename error_method_type
    >
    class handler
    {
    public:
      handler(
        done_method_type & done_method,
        next_method_type & next_method,
        error_method_type & error_method,
        const stream_to_string & args
      ):
      done_method_(done_method),
      next_method_(next_method),
      error_method_(error_method)
      {        
      }

      ~handler()
      {
        std::cout << "stream_to_string::handler::~handler\n";
      }
      
      void operator()(const void * data, size_t len)
      {
        if(0 == len){
          this->next_method_(this->body_);
        }
        else {
          this->body_.append(
            reinterpret_cast<const char *>(data),
            len);
          this->done_method_();
        }
      }
      
      void processed()
      {
        this->done_method_();
      }
      
    private:
      done_method_type & done_method_;
      next_method_type & next_method_;
      error_method_type & error_method_;
      std::string body_;
    };
  };

}

#endif // __VDS_CORE_STREAM_TO_STRING_H_
