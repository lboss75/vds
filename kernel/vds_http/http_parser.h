#ifndef __VDS_HTTP_HTTP_PARSER_H
#define __VDS_HTTP_HTTP_PARSER_H

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>
#include <list>
#include <string.h>

#include "func_utils.h"
#include "service_provider.h"
#include "http_request.h"
#include "http_incoming_stream.h"

namespace vds {
  class http_parser
  {
  public:
    http_parser();

    using incoming_item_type = uint8_t;
    using outgoing_item_type = std::shared_ptr<http_message>;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public sync_dataflow_filter<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_target<context_type, handler<context_type>>;

    public:
      handler(
        const context_type & context,
        const http_parser & args)
        : base_class(context),
        state_(STATE_PARSE_HEADER)
      {
      }


      void sync_process_data(const service_provider & sp, size_t & readed, size_t & written)
      {
        readed = 0;
        written = 0;
        while (0 < this->input_buffer_size_ && written < this->output_buffer_size_) {
          if (STATE_PARSE_HEADER == this->state_) {
            const char * p = (const char *)memchr(this->input_buffer_, '\n', this->input_buffer_size_);
            if (nullptr == p) {
              this->parse_buffer_ += std::string((const char *)this->input_buffer_, this->input_buffer_size_);
              readed += this->input_buffer_size_;
              return;
            }

            auto size = p - (const char *)this->input_buffer_;

            if (size > 0) {
              if ('\r' == reinterpret_cast<const char *>(this->input_buffer_)[size - 1]) {
                this->parse_buffer_ += std::string((const char *)this->input_buffer_, size - 1);
              }
              else {
                this->parse_buffer_ += std::string((const char *)this->input_buffer_, size);
              }
            }
            this->input_buffer_ = p + 1;
            this->input_buffer_size_ -= size + 1;

            if (0 == this->parse_buffer_.length()) {
              if (this->headers_.empty()) {
                throw std::logic_error("Invalid request");
              }

              this->current_message_ = std::make_shared<http_message>(this->headers_);
              this->output_buffer_[written++] = this->current_message_;

              std::string content_length_header;
              if (this->current_message_.get_header("Content-Length", content_length_header)) {
                this->content_length_ = std::stoul(content_length_header);
              }
              else {
                this->content_length_ = 0;
              }

              this->headers_.clear();

              if (0 < this->content_length_) {
                this->state_ = STATE_PARSE_BODY;
              }
            }
            else {
              this->headers_.push_back(this->parse_buffer_);
              this->parse_buffer_.clear();
            }
          }
          else {
            auto size = this->input_buffer_size_;
            if (size > this->content_length_) {
              size = this->content_length_;
            }

            if (0 < size) {
              auto p = this->input_buffer_;

              this->content_length_ -= size;
              this->input_buffer_ += size;
              this->input_buffer_size_ -= size;

              if (0 == this->content_length_) {
                this->state_ = STATE_PARSE_HEADER;
              }

              this->incoming_stream_.push_data(sp, p, size);
              return false;
            }
          }
        }

        return true;
      }

    private:
      std::string parse_buffer_;
      std::list<std::string> headers_;

      enum
      {
        STATE_PARSE_HEADER,
        STATE_PARSE_BODY
      } state_;

      size_t content_length_;
      
      std::shared_ptr<http_message> current_message_;
    };

    private:
      service_provider sp_;
  };
}

#endif // HTTP_PARSER_H
