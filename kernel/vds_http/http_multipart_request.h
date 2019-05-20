#ifndef __VDS_HTTP_HTTP_MULTIPART_REQUEST_H_
#define __VDS_HTTP_HTTP_MULTIPART_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"
#include "http_serializer.h"

#include <queue>

namespace vds {
  class http_client;

  class http_multipart_request {
  public:
    http_multipart_request(
      const std::string & method,
      const std::string & url,
      const std::string & agent = "HTTP/1.0");

    void add_string(
      const std::string & name,
      const std::string & value);

    expected<void> add_file(
      const std::string & name,
      const filename & body_file,
      const std::string & filename,
      const std::string & content_type = "application/octet-stream",
      const std::list<std::string> & headers = std::list<std::string>());


    async_task<expected<void>> send(
      const std::shared_ptr<http_async_serializer> & output_stream);

    async_task<expected<void>> send(
      const std::shared_ptr<http_client> & client,
      lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message && > && response_handler);

    void add_header(const std::string & header);

  private:
    std::list<std::string> headers_;
    std::string boundary_;
    size_t total_size_;
    std::queue<std::shared_ptr<stream_input_async<uint8_t>>> inputs_;
  };
}

#endif // __VDS_HTTP_HTTP_MULTIPART_REQUEST_H_
