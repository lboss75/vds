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
        const http_parser & args)
        : done_method_(done_method),
        next_method_(next_method),
        error_method_(error_method)
      {
      }
    
      
      void operator()(
        const void * data,
        size_t len
      ) {
        if (0 == len) {
          this->next_(this->request_, this->incoming_stream_);
        }
        else {
          this->data_ = data;
          this->len_ = len;

          this->processed();
        }
      }
      
      void processed()
      {
        if (!this->incoming_stream_) {
          while (0 < this->len_) {
            const char * p = (const char *)memchr(this->data_, '\n', this->len_);
            if (nullptr == p) {
              this->parse_buffer_ += std::string((const char *)this->data_, this->len_);
              this->done_method_();
              return;
            }

            auto size = p - data;

            if (size > 0) {
              if ('\r' == this->data_[size - 1]) {
                this->parse_buffer_ += std::string((const char *)this->data_, size - 1);
              }
              else {
                this->parse_buffer_ += std::string((const char *)this->data_, size);
              }
            }

            if (0 == this->parse_buffer_.length()) {
              if (this->headers_.empty()) {
                throw new std::logic_error("Invalid request");
              }

              auto request = *this->headers_.begin();

              std::string items[3];

              size_t index = 0;
              for (auto ch : request) {
                if (isspace(ch)) {
                  ++index;
                  if (index > sizeof(items) / sizeof(items[0])) {
                    throw new std::logic_error("Invalid request");
                  }
                }
                else {
                  items[index] += ch;
                }
              }

              if ((index + 1) != sizeof(items) / sizeof(items[0])) {
                throw new std::logic_error("Invalid request");
              }

              this->headers_.pop_front();

              this->request_.reset(
                  items[0],
                  items[1],
                  items[2],
                  this->headers_);

              this->data_ = p + 1;
              this->len_ -= size + 1;
              this->headers_.clear();

              this->next_method_(
                this->request_,
                this->incoming_stream_
              );

              return;
            }
            else {
              this->headers_.push_back(this->parse_buffer_);
              this->parse_buffer_.clear();
            }

            this->data_ = p + 1;
            this->len_ -= size + 1;
          }

          this->done_method_();
        }
        else {

        }
      }

    private:
      done_method_type & done_method_;
      next_method_type & next_method_;
      error_method_type & error_method_;
      
      const void * data_;
      size_t len_;

      std::string parse_buffer_;
      std::list<std::string> headers_;
      http_request request_;
      http_incoming_stream incoming_stream_;
    };
  };
}

#endif // HTTP_PARSER_H
