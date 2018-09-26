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
#include "barrier.h"
#include "state_machine.h"

namespace vds {

  template<typename implementation_class>
  class http_parser_base : public std::enable_shared_from_this<implementation_class> {
  public:
    http_parser_base(
      const std::function<std::future<void>(const http_message &message)> &message_callback)
      : message_callback_(message_callback) {
    }

    std::future<void> process(
      const service_provider & sp,
      const std::shared_ptr<stream_input_async<uint8_t>> & input_stream) {

      while (!this->eof_) {
        auto len = co_await input_stream->read_async(sp, this->buffer_, sizeof(this > buffer_));
        if (0 == len) {
          this->oef_ = true;
          break;
        }

        auto data = this->buffer_;

        char *p = (char *)memchr((const char *)data, '\n', len);
        if (nullptr == p) {
          this->parse_buffer_ += std::string((const char *)data, len);
          continue;
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
          if (this->headers_.empty()) {
            throw std::logic_error("Invalid request");
          }

          std::string transfer_encoding;
          const auto chunked_encoding = (http_message::get_header(this->headers_, "Transfer-Encoding", transfer_encoding) &&
            transfer_encoding == "chunked");

          std::string expect_value;
          const auto expect_100 = (http_message::get_header(this->headers_, "Expect", expect_value) &&
            "100-continue" == expect_value);

          size_t content_length;
          std::string content_length_header;
          if (http_message::get_header(this->headers_, "Content-Length", content_length_header)) {
            content_length = std::stoul(content_length_header);
          }
          else if (http_message::get_header(this->headers_, "Transfer-Encoding", transfer_encoding)) {
            content_length = (size_t)-1;
          }
          else {
            content_length = 0;
          }

          auto reader = std::make_shared<http_body_reader>(
            this->shared_from_this(),
            data,
            len,
            content_length,
            chunked_encoding,
            expect_100);

          http_message current_message(
            this->sp_,
            this->headers_,
            reader);

          this->headers_.clear();

          co_await this->process_message(current_message);

          this->eof_ = reader->get_rest_data(this->buffer_, sizeof(this->buffer_), len);
        }
      }

      co_await this->message_callback_(http_message());
    }


    std::future<void> continue_read_data(const service_provider &sp) {
    }

    std::future<void> finish_message(const service_provider &sp) {
    }

  private:
    std::function<std::future<void>(const http_message &message)> message_callback_;

    std::string parse_buffer_;
    std::list<std::string> headers_;

    uint8_t buffer_[1024];
    size_t readed_;

    class http_body_reader : public stream_input_async<uint8_t> {
    public:
      http_body_reader(
        http_parser_base * owner,
        const std::shared_ptr<stream_input_async<uint8_t>> & input_stream,
        size_t content_length,
        bool chunked_encoding,
        bool expect_100)
        : owner_(owner),
        input_stream_(input_stream),
        content_length_(content_length),
        chunked_encoding_(chunked_encoding),
        expect_100_(expect_100),
        readed_(0),
        processed_(0),
        eof_(false),
        state_(chunked_encoding ? StateEnum::STATE_PARSE_SIZE : StateEnum::STATE_PARSE_BODY) {
      }


      virtual std::future<size_t> read_async(
        const service_provider &sp,
        uint8_t * buffer,
        size_t buffer_size) {
        for (;;) {


          switch (this->state_) {
          case StateEnum::STATE_PARSE_BODY: {
            if (0 == this->content_length_) {
              if (this->chunked_encoding_) {
                this->state_ = StateEnum::STATE_PARSE_FINISH_CHUNK;
                this->parse_buffer_.clear();
              }
              else {
                static_cast<implementation_class *>(this->owner_)->finish_message(sp);
                co_return 0;
              }
            }

            if (this->readed_ == this->processed_) {
              this->processed_ = 0;
              this->readed_ = co_await this->input_stream_->read_async(this->buffer_, sizeof(this->buffer_));
              if (0 == this->readed_) {
                this->eof_ = true;
                co_return 0;
              }
            }

            auto size = this->readed_ - this->processed_;
            if (size > this->content_length_) {
              size = this->content_length_;
            }
            if (size > buffer_size) {
              size = buffer_size;
            }

            this->content_length_ -= size;
            memcpy(buffer, this->buffer_ + this->processed_, size);

            this->processed_ += size;

            co_return size;
          }
          case StateEnum::STATE_PARSE_SIZE: {
            if (this->readed_ == this->processed_) {
              this->processed_ = 0;
              this->readed_ = co_await this->input_stream_->read_async(this->buffer_, sizeof(this->buffer_));
              if (0 == this->readed_) {
                this->eof_ = true;
                co_return 0;
              }
            }

            char *p = (char *)memchr((const char *)this->buffer_ + this->processed_, '\n', this->readed_ - this->processed_);
            if (nullptr == p) {
              this->parse_buffer_ += std::string((const char *)this->buffer_ + this->processed_, this->readed_ - this->processed_);
              continue;
            }

            auto size = p - (const char *)this->buffer_ - this->processed_;

            if (size > 0) {
              if ('\r' == reinterpret_cast<const char *>(this->buffer_ + this->processed_)[size - 1]) {
                this->parse_buffer_ += std::string((const char *)(this->buffer_ + this->processed_), size - 1);
              }
              else {
                this->parse_buffer_.append((const char *)(this->buffer_ + this->processed_), size);
              }
            }

            this->processed_ += size + 1;

            this->content_length_ = 0;
            for (auto ch : this->parse_buffer_) {
              if ('0' <= ch && ch <= '9') {
                this->content_length_ <<= 4;
                this->content_length_ |= ch - '0';
              }
              else if ('a' <= ch && ch <= 'f') {
                this->content_length_ <<= 4;
                this->content_length_ |= ch - 'a' + 10;
              }
              else if ('A' <= ch && ch <= 'F') {
                this->content_length_ <<= 4;
                this->content_length_ |= ch - 'A' + 10;
              }
              else {
                throw std::runtime_error("Invalid size " + this->parse_buffer_);
              }
            }

            if (0 < this->content_length_) {
              this->state_ = StateEnum::STATE_PARSE_BODY;
            }
            else {
              this->state_ = StateEnum::STATE_PARSE_HEADER;
              co_return 0;
            }
            break;
          }
          case StateEnum::STATE_PARSE_FINISH_CHUNK: {
            if (this->buffer_[this->processed_] == '\r' && this->parse_buffer_.empty()) {
              this->parse_buffer_ += '\r';
              this->processed_++;
            }
            else if (this->buffer_[this->processed_] == '\n') {
              this->processed_++;
              this->state_ = StateEnum::STATE_PARSE_SIZE;
              co_await static_cast<implementation_class *>(this->owner_)->continue_read_data(sp);
            }
            else {
              throw std::runtime_error("Invalid data");
            }

            break;
          }
          default: {
            throw std::runtime_error("Invalid state");
          }
          }
        }
      }
    private:
      enum class StateEnum {
        STATE_PARSE_BODY,
        STATE_PARSE_SIZE,
        STATE_PARSE_FINISH_CHUNK,
      };

      http_parser_base * owner_;
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

  class http_parser : public http_parser_base<http_parser> {
  public:
    http_parser(
      const std::function<std::future<void>(const http_message &message)> &message_callback)
      : http_parser_base<http_parser>(message_callback) {
    }

    static std::string url_unescape(const std::string & string);
    static std::string url_escape(const std::string & string);
  };
}

#endif // __VDS_HTTP_HTTP_PARSER_H

