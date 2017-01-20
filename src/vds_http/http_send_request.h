#ifndef __VDS_HTTP_HTTP_SEND_REQUEST_H_
#define __VDS_HTTP_HTTP_SEND_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "http_request_serializer.h"

namespace vds {
  class http_request;
  class http_outgoing_stream;

  template <typename response_handler_type>
  class http_send_request
  {
  public:
    http_send_request(
      http_request & request,
      http_outgoing_stream & outgoing_stream,
      response_handler_type & response_handler
    ) : request_(request), outgoing_stream_(outgoing_stream),
      response_handler_(response_handler)
    {
    }

    template<
      typename done_method_type,
      typename error_method_type
    >
    class handler
    {
    public:
      handler(
        done_method_type & done_method,
        error_method_type & error_method,
        const http_send_request & args)
        :
        request_(args.request_),
        outgoing_stream_(args.outgoing_stream_),
        done_method_(done_method),
        error_method_(error_method),
        response_handler_(args.response_handler_)
      {
      }

      void operator()(const network_socket & s)
      {
        pipeline(
          http_request_serializer(this->request_, this->outgoing_stream_),
          output_network_stream(s)
        )
        (
          []() {},
          this->error_method_
        );

        pipeline(
          input_network_stream(s),
          this->response_handler_
        )(
          this->done_method_,
          this->error_method_
          );
      }

    private:
      done_method_type & done_method_;
      error_method_type & error_method_;

      http_request & request_;
      http_outgoing_stream & outgoing_stream_;
      response_handler_type & response_handler_;

      std::string header_;
    };

  private:
    http_request & request_;
    http_outgoing_stream & outgoing_stream_;
    response_handler_type & response_handler_;
  };
}

#endif // __VDS_HTTP_HTTP_SEND_REQUEST_H_
