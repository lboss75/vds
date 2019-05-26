/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_multipart_reader.h"

vds::http_multipart_reader::http_multipart_reader(
  const service_provider * sp,
  const std::string & boundary,
  lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>, http_message> part_handler,
  lambda_holder_t<vds::async_task<vds::expected<void>>> final_handler)
  : sp_(sp), boundary_("\r\n--" + boundary + "\r\n"), final_boundary_("\r\n--" + boundary + "--\r\n"),
  part_handler_(std::move(part_handler)), final_handler_(std::move(final_handler)),
  readed_(0) {
}

vds::async_task<vds::expected<void>> vds::http_multipart_reader::write_async(const uint8_t* data, size_t len) {
  if (0 == len) {
    if (this->readed_ != this->final_boundary_.length()
      || 0 != memcmp(this->buffer_, this->final_boundary_.c_str(), this->final_boundary_.length())) {
      co_return make_unexpected<std::runtime_error>("Invalid final block");
    }

    if (this->current_part_) {
      CHECK_EXPECTED_ASYNC(co_await this->current_part_->write_async(nullptr, 0));
      this->current_part_.reset();
    }

    CHECK_EXPECTED_ASYNC(co_await this->final_handler_());
    co_return expected<void>();
  }

  while (0 < len) {
    auto size = len;
    if (size > sizeof(this->buffer_) / sizeof(this->buffer_[0]) - this->readed_) {
      size = sizeof(this->buffer_) / sizeof(this->buffer_[0]) - this->readed_;
    }

    memcpy(this->buffer_ + this->readed_, data, size);
    data += size;
    len -= size;
    this->readed_ += size;


    for (;;) {
      std::string str((const char *)this->buffer_, this->readed_);

      auto p = str.find(this->boundary_);
      if (std::string::npos == p) {
        p = str.find(this->final_boundary_);
        if (std::string::npos == p) {
          if (this->readed_ > this->final_boundary_.length()) {
            CHECK_EXPECTED_ASYNC(co_await this->push_data(this->readed_ - this->final_boundary_.length()));
          }
        }
        else {
          if (0 != p) {
            CHECK_EXPECTED_ASYNC(co_await this->push_data(p));
          }

          vds_assert(this->readed_ == this->final_boundary_.length() && len == 0);
        }
        break;
      }
      else {
        if (0 != p) {
          CHECK_EXPECTED_ASYNC(co_await this->push_data(p));
        }

        this->readed_ -= this->boundary_.length();
        if (0 < this->readed_) {
          memmove(this->buffer_, this->buffer_ + this->boundary_.length(), this->readed_);
        }

        if (this->current_part_) {
          CHECK_EXPECTED_ASYNC(co_await this->current_part_->write_async(nullptr, 0));
          this->current_part_.reset();
        }
      }
    }
  }

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_multipart_reader::push_data(size_t size)
{
  vds_assert(size <= this->readed_);
  if (!this->current_part_) {
    this->current_part_ = std::make_shared<http_parser>(
      this->sp_,
      [pthis_ = this->shared_from_this()](http_message message)->vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>> {
      auto pthis = static_cast<http_multipart_reader *>(pthis_.get());
      return pthis->part_handler_(std::move(message));
    });
  }

  CHECK_EXPECTED_ASYNC(co_await this->current_part_->write_async(this->buffer_, size));
  
  this->readed_ -= size;
  if (this->readed_ > 0) {
    memmove(this->buffer_, this->buffer_ + size, this->readed_);
  }

  co_return expected<void>();
}
