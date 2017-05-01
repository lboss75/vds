#ifndef __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
#define __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_stream_reader.h"
#include "http_response.h"
#include "http_outgoing_stream.h"

namespace vds {
  class http_response;
  class http_outgoing_stream;

  class http_response_serializer
  {
  public:

    template<
      typename context_type
    >
      class handler : public dataflow_step<
        context_type,
        void(
          const void * data,
          size_t len
        )
      >
    {
      using base_class = dataflow_step<
        context_type,
        void(
          const void * data,
          size_t len
        )
      >;
    public:
      handler(
        const context_type & context,
        const http_response_serializer & args)
        : base_class(context)
      {
      }

      void operator()(
        const service_provider & sp,
        const http_response & response,
        http_outgoing_stream & response_stream
        )
      {
        if (!response.empty()) {
          std::stringstream stream;
          stream << "HTTP/1.0 " << response.code() << " " << response.comment() << "\n";
          for (auto & header : response.headers()) {
            stream << header << "\n";
          }

          stream
            << "Content-Length: " << response_stream.size() << "\n\n";

          if (response_stream.is_simple()) {
            stream << response_stream.body();
          }
          else {
            this->stream_.reset(new read_file_stream(response_stream.file()));
          }

          this->buffer_ = stream.str();
        }
        else {
          this->buffer_.clear();
        }
        this->next(
          sp,
          this->buffer_.c_str(),
          this->buffer_.size());
      }
      
      void processed(const service_provider & sp)
      {
        if (!this->stream_ || !this->stream_->read(this->next)) {
          this->prev(sp);
        }
      }

    private:
      std::string buffer_;
      std::unique_ptr<read_file_stream> stream_;
    };    
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
