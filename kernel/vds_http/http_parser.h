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
#include "http_message.h"

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
      using base_class = sync_dataflow_filter<context_type, handler<context_type>>;

    public:
      handler(
        const context_type & context,
        const http_parser & args)
        : base_class(context),
          state_(StateEnum::STATE_PARSE_HEADER)
      {
      }

      void sync_process_data(const service_provider & sp, size_t & readed, size_t & written)
      {
        readed = 0;
        written = 0;
        while (0 < this->input_buffer_size_ && written < this->output_buffer_size_) {
          if (StateEnum::STATE_PARSE_HEADER == this->state_) {
            char * p = (char *)memchr(this->input_buffer_, '\n', this->input_buffer_size_);
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
                this->parse_buffer_.append((const char *)this->input_buffer_, size);
              }
            }
            this->input_buffer_ = (uint8_t *)(p + 1);
            this->input_buffer_size_ -= size + 1;

            if (0 == this->parse_buffer_.length()) {
              if (this->headers_.empty()) {
                throw std::logic_error("Invalid request");
              }
              
              auto current_message = std::make_shared<http_message>(this->headers_, this->parse_buffer_);
              this->output_buffer_[written++] = current_message;
              this->headers_.clear();

              std::string content_length_header;
              if (current_message->get_header("Content-Length", content_length_header)) {
                this->content_length_ = std::stoul(content_length_header);
                
                if(this->content_length_ > 100 * 1024 * 1024){
                  this->error(sp, std::make_exception_ptr(std::runtime_error("request too long")));
                  return;
                }
              }
              else {
                this->content_length_ = 0;
              }

              if (0 < this->content_length_) {
                this->state_ = StateEnum::STATE_PARSE_BODY;
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
              
              this->parse_buffer_.append((const char *)this->input_buffer_, size);

              this->content_length_ -= size;
              this->input_buffer_ += size;
              this->input_buffer_size_ -= size;
              readed += size;

              if (0 == this->content_length_) {
                this->output_buffer_[written++] = std::make_shared<http_message>(this->headers_, this->parse_buffer_);
                this->headers_.clear();
                
                this->state_ = StateEnum::STATE_PARSE_HEADER;
              }
            }
          }
        }
      }

    private:
      std::string parse_buffer_;
      std::list<std::string> headers_;

      enum class StateEnum
      {
        STATE_PARSE_HEADER,
        STATE_PARSE_BODY
      };
      StateEnum state_;

      size_t content_length_;
    };
  };
}

#endif // HTTP_PARSER_H
