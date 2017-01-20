#ifndef __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_
#define __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "http_stream_reader.h"

namespace vds {
  class http_simple_response_reader
  {
  public:
    http_simple_response_reader(std::string & body)
    {
    }

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
        const http_request_serializer & args)
        : done_method_(done_method),
        next_method_(next_method),
        error_method_(error_method)
      {
      }

      void operator()(
        const http_response & response,
        const http_outgoing_stream & response_stream
        )
      {
        std::stringstream stream;
        stream << "HTTP/1.0 " << response.code() << " " << response.comment() << "\n";
        for (auto & header : response.headers()) {
          stream << header << "\n";
        }

        stream
          << "Content-Length: " << response_stream.length() << "\n"
          << "Connection: close\n\n";

        this->buffer_ = stream.str();
        response_stream.get_reader<next_method_type, error_method_type>(
          this->next_method_,
          this->error_method_,
          this->body_stream_);
        this->next_method_(this->buffer_.c_str(), this->buffer_.lenght());
      }

      void processed()
      {
        if(!this->body_stream_.read_async())
          this->done_method_();
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

#endif // __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_
