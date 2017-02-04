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
      class handler : public sequence_step<
        context_type,
        void(
          const void * data,
          size_t len
        )
      >
    {
      using base_class = sequence_step<
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
            << "Content-Length: " << response_stream.size() << "\n"
            << "Connection: close\n\n";

          if (response_stream.is_simple()) {
            stream << response_stream.body();
          }

          this->buffer_ = stream.str();
        }
        else {
          this->buffer_.clear();
        }

        this->next(
          this->buffer_.c_str(),
          this->buffer_.size());
      }
      
      void processed()
      {
        //Close connection
        this->next(
          nullptr,
          0);
      }

    private:
      std::string buffer_;
    };    
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
