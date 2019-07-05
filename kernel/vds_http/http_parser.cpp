/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_parser.h"

vds::http_parser::http_parser(
  const service_provider * sp,
  lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> message_callback) :
  sp_(sp), message_callback_(std::move(message_callback)), eof_(false), state_(StateEnum::STATE_PARSE_HEADER){
}

vds::http_parser::~http_parser() {

}

vds::async_task<vds::expected<void>> vds::http_parser::write_async(
  const uint8_t *data,
  size_t len) {

  vds_assert(!this->eof_);
  if (0 == len) {
    this->sp_->get<logger>()->trace("HTTPParser", "HTTP %p end", this);
    this->eof_ = true;

    switch(this->state_){
    case StateEnum::STATE_PARSE_HEADER: {
      /* Ignore */
      break;
    }
    case StateEnum::STATE_PARSE_SIZE:
    case StateEnum::STATE_PARSE_FINISH_CHUNK:
    case StateEnum::STATE_PARSE_FINISH_CHUNK1:
    case StateEnum::STATE_PARSE_BODY: {
      CHECK_EXPECTED_ASYNC(co_await this->finish_body());
      break;
    }
    }

    co_return co_await this->before_close();
  }
  this->sp_->get<logger>()->trace(
    "HTTPParser",
    "HTTP %p[%s],state=%d,content_length:%d,type:%s",
    this,
    std::string((const char *)data, len).c_str(),
    this->state_,
    this->content_length_,
    typeid(this).name());

  while (len != 0) {
    switch (this->state_) {
    case StateEnum::STATE_PARSE_HEADER: {
      char* p = (char *)memchr((const char *)data, '\n', len);
      if (nullptr == p) {
        this->parse_buffer_ += std::string((const char *)data, len);
        co_return expected<void>();
      }

      auto size = p - (const char *)data;

      if (size > 0) {
        this->parse_buffer_.append((const char *)data, size);
      }

      data += size + 1;
      len -= size + 1;

      if (!this->parse_buffer_.empty() && '\r' == this->parse_buffer_[this->parse_buffer_.length() -1]) {
        this->parse_buffer_.erase(this->parse_buffer_.length() - 1);
      }

      if (0 == this->parse_buffer_.length()) {
        if (this->headers_.empty()) {
          co_return vds::make_unexpected<std::logic_error>("Invalid request");
        }

        std::string transfer_encoding;
        this->chunked_encoding_ = (http_message::get_header(this->headers_, "Transfer-Encoding", transfer_encoding)
          && transfer_encoding == "chunked");

        std::string expect_value;
        this->expect_100_ = (http_message::get_header(this->headers_, "Expect", expect_value) &&
          "100-continue" == expect_value);

        http_message message(this->headers_);

        std::string content_length_header;
        if (http_message::get_header(this->headers_, "Content-Length", content_length_header)) {
          this->content_length_ = std::stoul(content_length_header);
        }
        else {
          this->content_length_ = (size_t)-1;
        }

        GET_EXPECTED_VALUE_ASYNC(this->current_message_, co_await this->message_callback_(std::move(message)));
        this->headers_.clear();
        if (!this->current_message_) {
          vds_assert(this->content_length_ == (size_t)-1);
          this->content_length_ = 0;
        }

        if (0 == this->content_length_) {
          CHECK_EXPECTED_ASYNC(co_await this->finish_body());
          continue;
        }
        else {
          this->state_ = this->chunked_encoding_ ? StateEnum::STATE_PARSE_SIZE : StateEnum::STATE_PARSE_BODY;
        }
      }
      else {
        this->headers_.push_back(this->parse_buffer_);
        this->parse_buffer_.clear();
      }
      break;
    }

    case StateEnum::STATE_PARSE_BODY: {
      if (0 == this->content_length_) {
        CHECK_EXPECTED_ASYNC(co_await this->finish_body());
        continue;
      }

      auto size = len;
      if (size > this->content_length_) {
        size = this->content_length_;
      }

      CHECK_EXPECTED_ASYNC(co_await this->current_message_->write_async(data, size));

      this->content_length_ -= size;
      data += size;
      len -= size;

      if (0 == this->content_length_) {
        CHECK_EXPECTED_ASYNC(co_await this->finish_body());
        continue;
      }

      continue;
    }

    case StateEnum::STATE_PARSE_SIZE: {
      char* p = (char *)memchr((const char *)data, '\n', len);
      if (nullptr == p) {
        this->parse_buffer_ += std::string((const char *)data, len);
        continue;
      }

      auto size = p - (const char *)data + 1;
      this->parse_buffer_.append((const char *)data, size);

      if (this->parse_buffer_.length() < 3
        || this->parse_buffer_[this->parse_buffer_.length() - 2] != '\r'
        || this->parse_buffer_[this->parse_buffer_.length() - 1] == '\n') {
        co_return make_unexpected<std::runtime_error>("Invalid protocol");
      }

      this->parse_buffer_.erase(this->parse_buffer_.length() - 2);

      data += size;
      len -= size;

      size_t pos;
      this->content_length_ = std::stoul(this->parse_buffer_, &pos, 16);
      if (pos < this->parse_buffer_.length()) {
        co_return make_unexpected<std::runtime_error>("Invalid size " + this->parse_buffer_);
      }

      if (0 < this->content_length_) {
        this->state_ = StateEnum::STATE_PARSE_BODY;
      }
      else {
        this->state_ = StateEnum::STATE_PARSE_HEADER;
      }

      break;
    }

    case StateEnum::STATE_PARSE_FINISH_CHUNK: {
      if ('\r' != *data) {
        co_return vds::make_unexpected<std::runtime_error>("Invalid data");
      }

      ++data;
      --len;
      this->state_ = StateEnum::STATE_PARSE_FINISH_CHUNK1;
      continue;
    }

    case StateEnum::STATE_PARSE_FINISH_CHUNK1: {
      if ('\n' != *data) {
        co_return vds::make_unexpected<std::runtime_error>("Invalid data");
      }

      ++data;
      --len;
      this->state_ = StateEnum::STATE_PARSE_SIZE;
      continue;
    }

    default: {
      co_return vds::make_unexpected<std::runtime_error>("Invalid state");
    }
    }
  }

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_parser::finish_body()
{
  if (this->current_message_) {
    CHECK_EXPECTED_ASYNC(co_await this->current_message_->write_async(nullptr, 0));
    this->current_message_.reset();
  }

  if (this->chunked_encoding_) {
    this->state_ = StateEnum::STATE_PARSE_FINISH_CHUNK;
    this->parse_buffer_.clear();
  }
  else {
    CHECK_EXPECTED_ASYNC(co_await this->finish_message());
    this->state_ = StateEnum::STATE_PARSE_HEADER;
  }

  co_return expected<void>();
}
