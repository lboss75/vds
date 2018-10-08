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

#include "func_utils.h"
#include "service_provider.h"
#include "http_message.h"
#include "barrier.h"
#include "state_machine.h"

namespace vds {

  template <typename implementation_class>
  class http_parser : public std::enable_shared_from_this<implementation_class>{
  public:
    http_parser(
      const std::function<std::future<void>(const http_message message)> &message_callback)
      : message_callback_(message_callback), eof_(false) {
    }

    std::future<void> process(
      
      const std::shared_ptr<stream_input_async<uint8_t>> & input_stream) {

      while (!this->eof_) {
        auto len = co_await input_stream->read_async(this->buffer_, sizeof(this->buffer_));
        if (0 == len) {
          this->eof_ = true;
          break;
        }

        auto data = this->buffer_;

        while (len != 0) {
          char *p = (char *)memchr((const char *)data, '\n', len);
          if (nullptr == p) {
            this->parse_buffer_ += std::string((const char *)data, len);
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
              input_stream,
              data,
              len,
              content_length,
              chunked_encoding,
              expect_100);

            http_message current_message(
              this->headers_,
              reader);

            this->headers_.clear();

            co_await this->message_callback_(current_message);

            this->eof_ = reader->get_rest_data(this->buffer_, sizeof(this->buffer_), len);
          }
          else {
            this->headers_.push_back(this->parse_buffer_);
            this->parse_buffer_.clear();
          }
        }
      }

      co_await this->message_callback_(http_message());
    }

    

    std::future<void> continue_read_data() {
      co_return;
    }

    std::future<void> finish_message() {
      co_return;
    }
    

  private:
    std::function<std::future<void>(const http_message message)> message_callback_;
    uint8_t buffer_[1024];
    size_t readed_;
    bool eof_;

    std::string parse_buffer_;
    std::list<std::string> headers_;


    class http_body_reader : public stream_input_async<uint8_t> {
    public:
      http_body_reader(
        const std::shared_ptr<http_parser> & owner,
        const std::shared_ptr<stream_input_async<uint8_t>> & input_stream,
        const uint8_t * data,
        size_t data_size,
        size_t content_length,
        bool chunked_encoding,
        bool expect_100)
        : owner_(owner),
        input_stream_(input_stream),
        content_length_(content_length),
        chunked_encoding_(chunked_encoding),
        expect_100_(expect_100),
        readed_(data_size),
        processed_(0),
        eof_(false),
        state_(chunked_encoding ? StateEnum::STATE_PARSE_SIZE : StateEnum::STATE_PARSE_BODY) {
        if(0 < data_size) {
          memcpy(this->buffer_, data, data_size);
        }
      }


      std::future<size_t> parse_body(
        
        uint8_t* buffer,
        size_t buffer_size) {
        if (0 == this->content_length_) {
          if (this->chunked_encoding_) {
            this->state_ = StateEnum::STATE_PARSE_FINISH_CHUNK;
            this->parse_buffer_.clear();
          }
          else {
            co_await static_cast<implementation_class *>(this->owner_.get())->finish_message();
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

      std::future<void> parse_content_size() {
        std::promise<void> result;

        size_t pos;
        this->content_length_ = std::stoul(this->parse_buffer_, &pos, 16);
        if (pos < this->parse_buffer_.length()) {
          result.set_exception(std::make_exception_ptr(std::runtime_error("Invalid size " + this->parse_buffer_)));
        }
        else {
          result.set_value();
        }

        return result.get_future();
      }

      std::future<size_t> read_async(
        
        uint8_t* buffer,
        size_t buffer_size) override {
        for (;;) {
          switch (this->state_) {
          case StateEnum::STATE_PARSE_BODY: {
            co_return co_await this->parse_body(buffer, buffer_size);
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

            co_await this->parse_content_size();

            if (0 < this->content_length_) {
              this->state_ = StateEnum::STATE_PARSE_BODY;
              break;
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
              co_await static_cast<implementation_class *>(this->owner_.get())->continue_read_data();
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


      bool get_rest_data(uint8_t* buffer, size_t buffer_size, size_t& rest_len) {
        rest_len = this->readed_ - this->processed_;
        if (0 < rest_len) {
          vds_assert(rest_len <= buffer_size);
          memcpy(buffer, this->buffer_ + this->processed_, rest_len);
        }
        return this->eof_;
      }

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

