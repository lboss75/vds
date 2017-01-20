#ifndef __VDS_HTTP_HTTP_SEND_REQUEST_H_
#define __VDS_HTTP_HTTP_SEND_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class http_request;
  class http_outgoing_stream;

  class http_send_request
  {
  public:
    http_send_request(
      http_request & request,
      http_outgoing_stream & outgoing_stream
    ) : request_(request), outgoing_stream_(outgoing_stream)
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
        const http_send_request & args)
        :
        request_(args.request_),
        outgoing_stream_(args.outgoing_stream_),
        next_method_(next_method),
        error_method_(error_method)
      {
        std::stringstream stream;
        stream << this->request_.method() << " " << this->request_.url() << " " << this->request_.agent() <<"\n";
        for (auto & header : this->request_.headers()) {
          stream << header << "\n";
        }

        stream << "\n";
        this->header_ = stream.str();
      }

      void operator()(network_socket && s)
      {
        pipeline(
          http_request_serializer(this->request_, this->outgoing_stream_),
          http_outgoing_stream(s)
        )
        (
          []() {},
          this->error_method_
        );

        pipeline(
          input_network_stream(s),
          http_response_deserializer(),

        this->write_task_.set_data(this->header_.c_str(), this->header_.length());
        this->write_task_.schedule(s.handle());
      }

    private:
      next_method_type & next_method_;
      error_method_type & error_method_;

      http_request & request_;
      http_outgoing_stream & outgoing_stream_;

      write_socket_task<next_method_type, error_method_type> write_task_;

      std::string header_;
    };

  private:
    http_request & request_;
    http_outgoing_stream & outgoing_stream_;
  };
}

#endif // __VDS_HTTP_HTTP_SEND_REQUEST_H_
