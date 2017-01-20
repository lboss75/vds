#ifndef __VDS_HTTP_HTTP_REQUEST_SERIALIZER_H_
#define __VDS_HTTP_HTTP_REQUEST_SERIALIZER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "http_stream_reader.h"

namespace vds {
  class http_response;
  class http_outgoing_stream;

  class http_request_serializer
  {
  public:
    http_request_serializer(
      http_request & request,
      http_outgoing_stream & outgoing_stream
    )
      : request_(request), outgoing_stream_(outgoing_stream)
    {
    }

    template<
      typename next_method_type,
      typename error_method_type
    >
      class handler
    {
    public:
      handler(
        next_method_type & next_method,
        error_method_type & error_method,
        const http_request_serializer & args)
        :
        next_method_(next_method),
        error_method_(error_method)
      {
        std::stringstream stream;
        stream << args.request_.method() << " " << args.request_.url() << " " << args.request_.agent() << "\n";
        for (auto & header : args.request_.headers()) {
          stream << header << "\n";
        }

        stream << "\n";
        this->header_ = stream.str();
      }

      void operator()()
      {
        this->next_method_(this->header_.c_str(), this->header_.lenght());
      }

    private:
      next_method_type & next_method_;
      error_method_type & error_method_;

      std::string header_;
    };

  private:
    http_request & request_;
    http_outgoing_stream & outgoing_stream_;
  };
}

#endif // __VDS_HTTP_HTTP_REQUEST_SERIALIZER_H_
