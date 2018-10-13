#ifndef __VDS_HTTP_HTTP_FORM_PART_PARSER_H
#define __VDS_HTTP_HTTP_FORM_PART_PARSER_H

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
#include "barrier.h"
#include "state_machine.h"

namespace vds {

  class http_form_part_parser {
  public:
    http_form_part_parser(
        const std::function<vds::async_task<void>(const http_message &message)> &message_callback)
    : message_callback_(message_callback) {
    }

    vds::async_task<void> start(
      const service_provider * sp,
      const std::shared_ptr<stream_input_async<uint8_t>> & input_stream) {
      for (;;) {
        auto len = co_await input_stream->read_async(this->buffer_, sizeof(this->buffer_));
        if (0 == len) {
          logger::get(sp)->debug("HTTP", "HTTP end");
          co_return;
        }
        else {
          auto data = this->buffer_;
          while (len > 0) {
            char *p = (char *)memchr((const char *)data, '\n', len);
            if (nullptr == p) {
              this->parse_buffer_ += std::string((const char *)data, len);
              len = 0;
              break;
            }

            auto size = p - (const char *)data;

            if (size > 0) {
              if ('\r' == reinterpret_cast<const char *>(data)[size - 1]) {
                this->parse_buffer_ += std::string((const char *)data, size - 1);
              }
              else {
                this->parse_buffer_.append((const char *)data, size);
              }
            }

            data += size + 1;
            len -= size + 1;

            if (0 == this->parse_buffer_.length()) {
              this->current_message_body_ = std::make_shared<message_body_reader>(input_stream, data, len);
              this->current_message_ = http_message(this->headers_, this->current_message_body_);
              this->headers_.clear();

              co_await this->message_callback_(this->current_message_);

              static_cast<message_body_reader *>(this->current_message_body_.get())->get_rest_data(this->buffer_, len);
              data = this->buffer_;
            }
            else {
              this->headers_.push_back(this->parse_buffer_);
              this->parse_buffer_.clear();
            }
          }
        }
      }
    }


  private:
    std::function<vds::async_task<void>(const http_message &message)> message_callback_;

    uint8_t buffer_[1024];
    std::string parse_buffer_;
    std::list<std::string> headers_;
    http_message current_message_;
    std::shared_ptr<stream_input_async<uint8_t>> current_message_body_;

    class message_body_reader : public stream_input_async<uint8_t> {
    public:
      message_body_reader(
        const std::shared_ptr<stream_input_async<uint8_t>> & source,
        const uint8_t * data,
        size_t size)
      :source_(source), processed_(0), eof_(false){
        if(0 != size) {
          memcpy(this->buffer_, data, size);
          this->readed_ = size;
        }
        else {
          this->readed_ = 0;
        }
        
      }

      vds::async_task<size_t> read_async( uint8_t * buffer, size_t len) override {
        vds_assert(!this->eof_);

        if (this->readed_ <= this->processed_) {
          this->processed_ = 0;
          this->readed_ = co_await this->source_->read_async(this->buffer_, sizeof(this->buffer_));
          if (this->readed_ == 0) {
            this->eof_ = true;
            co_return 0;
          }
        }

        if (len > this->readed_ - this->processed_) {
          len = this->readed_ - this->processed_;
        }

        memcpy(buffer, this->buffer_ + this->processed_, len);
        this->processed_ += len;
        co_return len;
      }

      void get_rest_data(uint8_t * data, size_t & len) {
        if (this->readed_ > this->processed_) {
          len = this->readed_ - this->processed_;
          memcpy(data, this->buffer_ + this->processed_, len);
        }
        else {
          len = 0;
        }
      }

    private:
      std::shared_ptr<stream_input_async<uint8_t>> source_;
      uint8_t buffer_[1024];
      size_t processed_;
      size_t readed_;
      bool eof_;
    };
  };
}

#endif // __VDS_HTTP_HTTP_FORM_PART_PARSER_H

