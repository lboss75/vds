#ifndef __VDS_CORE_STREAM_TO_STRING_H_
#define __VDS_CORE_STREAM_TO_STRING_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "dataflow.h"

namespace vds {
  
  class stream_to_string
  {
  public:
    stream_to_string()
    {
    }
    
    template <
      typename conext_type
    >
    class handler : public dataflow_step<
      conext_type,
      void(const std::string &)
    >    
    {
      using base_class = dataflow_step<
        conext_type,
        void(const std::string &)
      > ;
    public:
      handler(
        const conext_type & context,
        const stream_to_string & args
      ): base_class(context)
      {        
      }

      ~handler()
      {
        std::cout << "stream_to_string::handler::~handler\n";
      }
      
      void operator()(const void * data, size_t len)
      {
        if(0 == len){
          this->next(this->body_);
        }
        else {
          this->body_.append(
            reinterpret_cast<const char *>(data),
            len);
          this->prev();
        }
      }
      
    private:
      std::string body_;
    };
  };

}

#endif // __VDS_CORE_STREAM_TO_STRING_H_
