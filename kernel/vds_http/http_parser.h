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
    http_parser(const service_provider & sp);

    template<
      typename context_type
    >
    class handler : public sequence_step<
      context_type,
      void(
        http_request & request,
        http_incoming_stream & incoming_stream
      )
    >
    {
      using base_class = sequence_step<
        context_type,
        void(
          http_request & request,
          http_incoming_stream & incoming_stream
        )
      >;
    public:
      handler(
        const context_type & context,
        const http_parser & args)
        : base_class(context),
        state_(STATE_PARSE_HEADER),
        sp_(args.sp_), log_(args.sp_, "HTTP Parser")
      {
      }
    
      
      void operator()(
        const void * data,
        size_t len
      ) {
        if (0 == len) {
          this->next(
            this->request_,
            this->incoming_stream_);
        }
        else {
          this->data_ = data;
          this->len_ = len;

          this->processed();
        }
      }
      
      void processed()
      {
        while (0 < this->len_) {
          if (STATE_PARSE_HEADER == this->state_) {
            const char * p = (const char *)memchr(this->data_, '\n', this->len_);
            if (nullptr == p) {
              this->parse_buffer_ += std::string((const char *)this->data_, this->len_);
              this->prev();
              return;
            }

            auto size = p - (const char *)this->data_;

            if (size > 0) {
              if ('\r' == reinterpret_cast<const char *>(this->data_)[size - 1]) {
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

              if (index < 1) {
                throw new std::logic_error("Invalid request");
              }

              this->headers_.pop_front();

              this->request_.reset(
                items[0],
                items[1],
                items[2],
                this->headers_);

              this->log_(
                ll_trace,
                "Request url:%s, method: %s, agent:%s",
                this->request_.url().c_str(),
                this->request_.method().c_str(),
                this->request_.agent().c_str());
              
              for (auto & p : this->headers_) {
                this->log_(ll_trace, p);
              }

              std::string content_length_header;
              if (this->request_.get_header("Content-Length", content_length_header)) {
                this->content_length_ = std::stoul(content_length_header);
              }
              else {
                this->content_length_ = 0;
              }

              this->data_ = p + 1;
              this->len_ -= size + 1;
              this->headers_.clear();

              if (0 < this->content_length_) {
                this->state_ = STATE_PARSE_BODY;
              }

              this->next(
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
          else {
            auto size = this->len_;
            if (size > this->content_length_) {
              size = this->content_length_;
            }

            if (0 < size) {
              this->incoming_stream_.push_data(this->data_, size);
            }

            this->content_length_ -= size;
            this->data_ = reinterpret_cast<const char *>(this->data_) + size;
            this->len_ -= size;

            if (0 == this->content_length_) {
              this->state_ = STATE_PARSE_HEADER;
            }
          }
        }

        this->prev();
      }

    private:
      const void * data_;
      size_t len_;

      std::string parse_buffer_;
      std::list<std::string> headers_;
      
      enum 
      {
        STATE_PARSE_HEADER,
        STATE_PARSE_BODY
      } state_;
      
      http_request request_;
      http_incoming_stream incoming_stream_;
      size_t content_length_;

      const service_provider & sp_;
      logger log_;
    };
    private:
      const service_provider & sp_;
  };
}

#endif // HTTP_PARSER_H
