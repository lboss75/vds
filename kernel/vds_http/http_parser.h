#ifndef __VDS_HTTP_HTTP_PARSER_H
#define __VDS_HTTP_HTTP_PARSER_H

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>
#include <list>
#include <optional>
#include <string.h>

#include "service_provider.h"
#include "http_message.h"

namespace vds {

  class http_parser : public std::enable_shared_from_this<http_parser>{
  public:
    http_parser(
      const std::function<vds::async_task<vds::expected<void>>(http_message message)>& message_callback);

    ~http_parser();

    async_task<expected<void>> process(
      const std::shared_ptr<stream_input_async<uint8_t>>& input_stream);

    async_task<expected<void>> call_message_callback(http_message message) const;

    virtual async_task<expected<void>> continue_read_data() {
      co_return expected<void>();
    }

    virtual async_task<expected<void>> finish_message() {
      co_return expected<void>();
    }
    

  private:
    std::function<vds::async_task<vds::expected<void>>(http_message message)> message_callback_;
    uint8_t buffer_[1024];
    size_t readed_;
    bool eof_;

    std::string parse_buffer_;
    std::list<std::string> headers_;


    class http_body_reader : public stream_input_async<uint8_t> {
    public:
      http_body_reader(
        const std::shared_ptr<http_parser>& owner,
        const std::shared_ptr<stream_input_async<uint8_t>>& input_stream,
        const uint8_t* data,
        size_t data_size,
        size_t content_length,
        bool chunked_encoding,
        bool expect_100);

      vds::async_task<vds::expected<size_t>> parse_body(
        uint8_t* buffer,
        size_t buffer_size);

      async_task<expected<void>> parse_content_size();

      async_task<expected<size_t>> read_async(
        uint8_t* buffer,
        size_t buffer_size) override;


      bool get_rest_data(uint8_t* buffer, size_t buffer_size, size_t& rest_len);

    private:
      enum class StateEnum {
        STATE_PARSE_HEADER,
        STATE_PARSE_BODY,
        STATE_PARSE_SIZE,
        STATE_PARSE_FINISH_CHUNK,
      };

      std::shared_ptr<http_parser> owner_;
      std::shared_ptr<stream_input_async<uint8_t>> input_stream_;
      size_t content_length_;
      bool chunked_encoding_;
      bool expect_100_;

      uint8_t buffer_[1024];
      size_t readed_;
      size_t processed_;
      bool eof_;

      std::string parse_buffer_;
      StateEnum state_;
    };
  };
}

#endif // __VDS_HTTP_HTTP_PARSER_H

