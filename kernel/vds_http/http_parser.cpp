/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_parser.h"

vds::http_parser::http_parser(
  const std::function<vds::async_task<vds::expected<void>>(http_message message)>& message_callback):
  message_callback_(message_callback), eof_(false) {
}

vds::http_parser::~http_parser() {

}

vds::async_task<vds::expected<void>> vds::http_parser::process(
  const std::shared_ptr<stream_input_async<uint8_t>>& input_stream) {

  while (!this->eof_) {
    GET_EXPECTED_ASYNC(len, co_await input_stream->read_async(this->buffer_, sizeof(this->buffer_)));
    if (0 == len) {
      this->eof_ = true;
      break;
    }

    auto data = this->buffer_;

    while (len != 0) {
      char* p = (char *)memchr((const char *)data, '\n', len);
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
          co_return vds::make_unexpected<std::logic_error>("Invalid request");
        }

        std::string transfer_encoding;
        const auto chunked_encoding = (http_message::get_header(this->headers_, "Transfer-Encoding", transfer_encoding)
          &&
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

        CHECK_EXPECTED_ASYNC(co_await this->call_message_callback(current_message));

        this->eof_ = reader->get_rest_data(this->buffer_, sizeof(this->buffer_), len);
      }
      else {
        this->headers_.push_back(this->parse_buffer_);
        this->parse_buffer_.clear();
      }
    }
  }

  CHECK_EXPECTED_ASYNC(co_await this->call_message_callback(http_message()));
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_parser::call_message_callback(const http_message message) const {
  return this->message_callback_(message);
}

vds::http_parser::http_body_reader::http_body_reader(const std::shared_ptr<http_parser>& owner,
                                                     const std::shared_ptr<stream_input_async<uint8_t>>& input_stream,
                                                     const uint8_t* data, size_t data_size,
                                                     size_t content_length, bool chunked_encoding,
                                                     bool expect_100): owner_(owner),
                                                                       input_stream_(input_stream),
                                                                       content_length_(content_length),
                                                                       chunked_encoding_(chunked_encoding),
                                                                       expect_100_(expect_100),
                                                                       readed_(data_size),
                                                                       processed_(0),
                                                                       eof_(false),
                                                                       state_(chunked_encoding
                                                                                ? StateEnum::STATE_PARSE_SIZE
                                                                                : StateEnum::STATE_PARSE_BODY) {
  if (0 < data_size) {
    memcpy(this->buffer_, data, data_size);
  }
}

vds::async_task<vds::expected<size_t>> vds::http_parser::http_body_reader::parse_body(uint8_t* buffer,
                                                                                      size_t buffer_size) {
  if (0 == this->content_length_) {
    if (this->chunked_encoding_) {
      this->state_ = StateEnum::STATE_PARSE_FINISH_CHUNK;
      this->parse_buffer_.clear();
    }
    else {
      CHECK_EXPECTED_ASYNC(co_await this->owner_.get()->finish_message());
      co_return 0;
    }
  }

  if (this->readed_ == this->processed_) {
    this->processed_ = 0;
    GET_EXPECTED_VALUE_ASYNC(this->readed_, co_await this->input_stream_->read_async(this->buffer_, sizeof(this->buffer_
    )));
    if (0 == this->readed_) {
      vds_assert(0 == this->content_length_);
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

vds::async_task<vds::expected<void>> vds::http_parser::http_body_reader::parse_content_size() {
  async_result<expected<void>> result;

  size_t pos;
  this->content_length_ = std::stoul(this->parse_buffer_, &pos, 16);
  if (pos < this->parse_buffer_.length()) {
    result.set_value(make_unexpected<std::runtime_error>("Invalid size " + this->parse_buffer_));
  }
  else {
    result.set_value(expected<void>());
  }

  return result.get_future();
}

vds::async_task<vds::expected<unsigned>> vds::http_parser::http_body_reader::read_async(uint8_t* buffer,
                                                                                        size_t buffer_size) {
  for (;;) {
    switch (this->state_) {
    case StateEnum::STATE_PARSE_BODY: {
      GET_EXPECTED_ASYNC(parse_result, co_await this->parse_body(buffer, buffer_size));
      co_return parse_result;
    }
    case StateEnum::STATE_PARSE_SIZE: {
      if (this->readed_ == this->processed_) {
        this->processed_ = 0;
        GET_EXPECTED_VALUE_ASYNC(
          this->readed_,
          co_await this->input_stream_->read_async(this->buffer_, sizeof(this->buffer_)));
        if (0 == this->readed_) {
          this->eof_ = true;
          co_return 0;
        }
      }

      char* p = (char *)memchr((const char *)this->buffer_ + this->processed_, '\n', this->readed_ - this->processed_);
      if (nullptr == p) {
        this->parse_buffer_ += std::string((const char *)this->buffer_ + this->processed_,
                                           this->readed_ - this->processed_);
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

      CHECK_EXPECTED_ASYNC(co_await this->parse_content_size());

      if (0 < this->content_length_) {
        this->state_ = StateEnum::STATE_PARSE_BODY;
        break;
      }
      else {
        this->state_ = StateEnum::STATE_PARSE_HEADER;
        co_return 0;
      }

    }
    case StateEnum::STATE_PARSE_FINISH_CHUNK: {
      if (this->buffer_[this->processed_] == '\r' && this->parse_buffer_.empty()) {
        this->parse_buffer_ += '\r';
        ++this->processed_;
      }
      else if (this->buffer_[this->processed_] == '\n') {
        ++this->processed_;
        this->state_ = StateEnum::STATE_PARSE_SIZE;
        CHECK_EXPECTED_ASYNC(co_await this->owner_.get()->continue_read_data());
      }
      else {
        co_return vds::make_unexpected<std::runtime_error>("Invalid data");
      }

      break;
    }
    default: {
      co_return vds::make_unexpected<std::runtime_error>("Invalid state");
    }
    }
  }
}

bool vds::http_parser::http_body_reader::get_rest_data(uint8_t* buffer, size_t buffer_size, size_t& rest_len) {
  rest_len = this->readed_ - this->processed_;
  if (0 < rest_len) {
    vds_assert(rest_len <= buffer_size);
    memcpy(buffer, this->buffer_ + this->processed_, rest_len);
  }
  return this->eof_;
}
