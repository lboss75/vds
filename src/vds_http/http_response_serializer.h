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
        const http_response_serializer & args)
        : done_method_(done_method),
        next_method_(next_method),
        error_method_(error_method)
      {
      }

      void operator()(
        const http_response & response,
        http_outgoing_stream & response_stream
        )
      {
        std::stringstream stream;
        stream << "HTTP/1.0 " << response.code() << " " << response.comment() << "\n";
        for (auto & header : response.headers()) {
          stream << header << "\n";
        }

        stream
          << "Content-Length: " << response_stream.size() << "\n"
          << "Connection: close\n\n";

        this->buffer_ = stream.str();
        response_stream.get_reader<next_method_type, error_method_type>(
          this->next_method_,
          this->error_method_,
          this->body_stream_);
        this->next_method_(this->buffer_.c_str(), this->buffer_.size());
      }

      void processed()
      {
        if(!this->body_stream_.read_async()) {          
          this->done_method_();
        }
      }

    private:
      done_method_type & done_method_;
      next_method_type & next_method_;
      error_method_type & error_method_;

      std::string buffer_;
      http_stream_reader<next_method_type, error_method_type> body_stream_;
    };    
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
