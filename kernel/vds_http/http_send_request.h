#ifndef __VDS_HTTP_HTTP_SEND_REQUEST_H_
#define __VDS_HTTP_HTTP_SEND_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "http_request_serializer.h"
#include "http_response_parser.h"

namespace vds {
  class http_request;
  class http_outgoing_stream;

  template <typename response_handler_type>
  class http_send_request
  {
  public:
    http_send_request(
      const service_provider & sp,
      http_request & request,
      http_outgoing_stream & outgoing_stream,
      response_handler_type & response_handler
    ) : sp_(sp),
      request_(request), outgoing_stream_(outgoing_stream),
      response_handler_(response_handler)
    {
    }

    template<typename context_type>
    class handler : public sequence_step<context_type, void(const std::string &)>
    {
      using base_class = sequence_step<context_type, void(const std::string &)>;
    public:
      handler(
        const context_type & context,
        const http_send_request & args)
        : base_class(context),
        sp_(args.sp_),
        request_(args.request_),
        outgoing_stream_(args.outgoing_stream_),
        response_handler_(args.response_handler_)
      {
      }
      
      ~handler()
      {
        std::cout << "http_send_request::handler::~handler\n";
      }

      void operator()(const network_socket & s)
      {
        sequence(
          http_request_serializer(),
          output_network_stream(this->sp_, s)
        )
        (
          this->request_sent_handler_,
          this->error,
          this->request_,
          this->outgoing_stream_
        );

        sequence(
          input_network_stream(this->sp_, s),
          http_response_parser(),
          this->response_handler_
        )(
          this->next,
          this->error
        );
      }

    private:
      class request_sent
      {
      public:
        ~request_sent()
        {
        }
        
        void operator()()
        {
          std::cout << "request sent\n";
        }
      };
      
      service_provider sp_;
      request_sent request_sent_handler_;
      http_request & request_;
      http_outgoing_stream & outgoing_stream_;
      response_handler_type & response_handler_;

      std::string header_;
    };

  private:
    service_provider sp_;
    http_request & request_;
    http_outgoing_stream & outgoing_stream_;
    response_handler_type & response_handler_;
  };
}

#endif // __VDS_HTTP_HTTP_SEND_REQUEST_H_
